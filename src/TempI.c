#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<gtk/gtk.h>
#include<libappindicator/app-indicator.h>

#include "debugger.h"
#include "TempI.h"

int main(int argc, char **argv){
	//Initialize TempI_Main
	TempI_Main_t TempI_Main;
	//This way we don't need to fill the TempI_CPU_Core_Structs inside TempI_Main with
	//0's since they are included in sizeof(TempI_Main_t).
	memset(&TempI_Main, 0, sizeof(TempI_Main_t));
	
	//Executable path is needed for reading the configuration file and loading icons for the indicator
	TempI_Main.ExecutablePath = TempI_Resolve_Executable_Path();
	D_check(TempI_Main.ExecutablePath != NULL, "Couldn't resolve executable path.");
	
	D_check(TempI_Read_Config(&TempI_Main) == 0, "Couldn't read configuration file.");
	
	TempI_Main.Cores_Counter = TempI_Number_Of_Cores();
	D_debug("Number of Cores: %u.", TempI_Main.Cores_Counter);
	D_check(TempI_Main.Cores_Counter <= TEMPI_MAX_CORES, "The number of cores exceeds the limit.");
	
	//LogFile
	if(TempI_Main.Log == 1){
		TempI_Main.LogFile = fopen(TempI_Main.LogFileName, "w");
	}
	
	//Starts gtk
	gtk_init(&argc, &argv);
	
	D_check(TempI_Resolve_Icons_Path(&TempI_Main) == 0, "Couldn't resolve indicator's icons paths.");
	
	//Set indicators
	TempI_Set_Main_Indicator(&TempI_Main);
	for (int i=0; i < TempI_Main.Cores_Counter; i++){
		TempI_Set_Core_Indicator(&TempI_Main.Core[i], TempI_Main.Gtk_Core_Icon_Path[0], i);
	}
	
	//Update routine will be called as a timeout on Glib
	g_timeout_add(TempI_Main.Delay*1000, TempI_Update, &TempI_Main);
	
	gtk_main();
	
	TempI_Free_Everything(&TempI_Main);
	return 0;

error:
	TempI_Free_Everything(&TempI_Main);
	return -1;
}

gint TempI_Update(gpointer datapointer){
	//Cast pointer
	TempI_Main_t *self;
	self = (TempI_Main_t *) datapointer; //Is it the right way?
	
	D_check(TempI_Get_Core_Temperatures(self) == 0, "Couldn't update core temperatures.");

	//Used to make the log file time stamps
	time_t Rawtime;
	struct tm *TimeStruct;
	char *TimeStamp;
	
	time(&Rawtime);
	TimeStruct = localtime(&Rawtime);
	TimeStamp = asctime(TimeStruct);
	
	if(self->Log == 1){
		fprintf(self->LogFile, "%s", TimeStamp);
	}
	
	//Updates status and sets the appropriate icon depending on the core status
	for(int i=0; i < self->Cores_Counter; i++){
		int TempRange = self->Core[i].High_Value-TEMPI_COOL;
		int Status2Temp = self->Core[i].High_Value-(TempRange/3);
		int Status1Temp = self->Core[i].High_Value-(2*TempRange/3);
		
		D_debug("Status2 Temp: %u\nStatus1 Temp: %u", Status2Temp, Status1Temp);
		
		if(self->Core[i].Temperature > self->Core[i].High_Value){
			self->Core[i].Status = 3;
		} else if(self->Core[i].Temperature > Status2Temp && self->Core[i].Temperature <= self->Core[i].High_Value){
			self->Core[i].Status = 2;
		} else if(self->Core[i].Temperature > Status1Temp && self->Core[i].Temperature <= Status2Temp){
			self->Core[i].Status = 1;
		} else {
			self->Core[i].Status = 0;
		}
		
		snprintf(self->Core[i].Gtk_Indicator_Label, 20, "Core %u: %uºC", i, self->Core[i].Temperature);
		
		app_indicator_set_icon(self->Core[i].Gtk_Indicator, self->Gtk_Core_Icon_Path[self->Core[i].Status]);
		app_indicator_set_label(self->Core[i].Gtk_Indicator, self->Core[i].Gtk_Indicator_Label,NULL);
		
		if(self->Log == 1){
			fprintf(self->LogFile, "Core %u:\n\tTemperature=%uºC\n\tHigh Temperature=%uºC\n\tCritical Temperature=%uºC\n",
					i, self->Core[i].Temperature, self->Core[i].High_Value, self->Core[i].Critical_Value);
		}
	}
	
	if(self->Log == 1){
		fflush(self->LogFile);
	}
	
	return TRUE;
error:
	return FALSE;
}

int TempI_Get_Core_Temperatures(TempI_Main_t *self){
	int NoC = 0; //Counter to check which core are we reading output from
	FILE *sensors_proc;
	char sensor_line[TEMPI_MAX_CHARS];
	
	sensors_proc = popen("sensors","r");
	D_check_pointer(sensors_proc);
	
	while(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
		if(strncmp(sensor_line, "Adapter: Virtual device", 23) == 0){
			//Ignore virtual device
			for(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc); sensor_line[0] != '\n';
				fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
				//Ignored
			}
		} else if(strncmp(sensor_line, "Adapter:", 8) == 0){
			//Extract temperature, high temperature and critical temperature for each core
			//Output will be something like:
			//Core 0:       +57.0°C  (high = +95.0°C, crit = +105.0°C)
			//Core 2:       +54.0°C  (high = +95.0°C, crit = +105.0°C)

			NoC = 0;
			
			for(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc); sensor_line[0] != '\n';
				fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
				
				char *string_pointer;
				char Temperature_String[10]; //Doesn't have to be that big, but just in case your CPU is as hot as the sun...
				char HighTemperature_String[10];
				char CriticalTemperature_String[10];
				
				D_debug("Sensors line: %s", sensor_line);
				
				//Goes to the first +, right behind the temperature
				string_pointer = strchr(sensor_line, '+');
				D_check(string_pointer != NULL, "Invalid sensors input.");
				
				string_pointer++; //Pointing at the first digit of temperature
				
				for(int i=0; i < 9 && *string_pointer != '.'; i++){
					Temperature_String[i] = *string_pointer;
					Temperature_String[i+1] = '\0'; //Making sure we end the string properly
					string_pointer++;
				}
				//Now string_pointer is at the decimals.
				while(*string_pointer != '+'){					
					string_pointer++;
				}
				
				string_pointer++; //Pointing at the first digit of high temperature
				
				for(int i=0; i < 9 && *string_pointer != '.'; i++){
					HighTemperature_String[i] = *string_pointer;
					HighTemperature_String[i+1] = '\0'; //Making sure we end the string properly
					string_pointer++;
				}
				//Now string_pointer is at the decimals.
				while(*string_pointer != '+'){					
					string_pointer++;
				}
				
				string_pointer++; //Pointing at the first digit of high temperature
				
				for(int i=0; i < 9 && *string_pointer != '.'; i++){
					CriticalTemperature_String[i] = *string_pointer;
					CriticalTemperature_String[i+1] = '\0'; //Making sure we end the string properly
					string_pointer++;
				}

				self->Core[NoC].Temperature = atoi(Temperature_String);
				self->Core[NoC].High_Value = atoi(HighTemperature_String);
				self->Core[NoC].Critical_Value = atoi(CriticalTemperature_String);
				
				D_debug("Temperature String: %s", Temperature_String);
				D_debug("HighTemperature String: %s", HighTemperature_String);
				D_debug("CriticalTemperature String: %s", CriticalTemperature_String);
				
				D_debug("Values on Core: T: %u - H: %u - C: %u", self->Core[NoC].Temperature,
					self->Core[NoC].High_Value, self->Core[NoC].Critical_Value);
				
				NoC++;
			}
		}
	}
	
	pclose(sensors_proc);
	
	return 0;
error:
	if(sensors_proc){
		pclose(sensors_proc);
	}
	
	return -1;
}

char *TempI_Concatenate_Path(const char *root, const char *subdir){
	size_t PathSize = strlen(root) + strlen(subdir) + 1;
	char *FinalPath;
	
	FinalPath = malloc(PathSize);
	
	D_check_pointer(FinalPath);
	
	strncpy(FinalPath, root, strlen(root) + 1); //+1 to include the terminating null byte
	
	strncat(FinalPath, subdir, PathSize - strlen(FinalPath) - 1);	//FinalPath must be strlen(FinalPath)+n+1 bytes long (PathSize)
																	//n = PathSize - strlen(FinalPath) - 1
																	//n+1+strlen(FinalPath) => PathSize (no buffer overflow!)
	return FinalPath;
error:
	return NULL;
}

int TempI_Resolve_Icons_Path(TempI_Main_t *self){
	//Get the main icon path
	self->Gtk_Indicator_Icon_Path = TempI_Concatenate_Path(self->ExecutablePath, "/Icons/menu-icon.png");
	
	D_check_pointer(self->Gtk_Indicator_Icon_Path);
		
	//Resolve Core states icon paths
	self->Gtk_Core_Icon_Path[0] = TempI_Concatenate_Path(self->ExecutablePath, "/Icons/green-icon.png");
		
	D_check_pointer(self->Gtk_Core_Icon_Path[0]);
	
	self->Gtk_Core_Icon_Path[1] = TempI_Concatenate_Path(self->ExecutablePath, "/Icons/yellow-icon.png");
	
	D_check_pointer(self->Gtk_Core_Icon_Path[1]);
	
	self->Gtk_Core_Icon_Path[2] = TempI_Concatenate_Path(self->ExecutablePath, "/Icons/orange-icon.png");
	
	D_check_pointer(self->Gtk_Core_Icon_Path[2]);
	
	self->Gtk_Core_Icon_Path[3] = TempI_Concatenate_Path(self->ExecutablePath, "/Icons/red-icon.png");
	
	D_check_pointer(self->Gtk_Core_Icon_Path[3]);
	
	//Prompt the paths as debugging
	D_debug("Main Indicator Icon Path: %s", self->Gtk_Indicator_Icon_Path);
	D_debug("Core Icon Paths:\n%s\n%s\n%s\n%s", self->Gtk_Core_Icon_Path[0], self->Gtk_Core_Icon_Path[1],
		self->Gtk_Core_Icon_Path[2], self->Gtk_Core_Icon_Path[3]);
	
	return 0;
error:
	return -1;
}

//This function sets the Gtk widgets and app indicator for each core
void TempI_Set_Core_Indicator(TempI_CPU_Core *core, char *icon, int core_number){
		core->Gtk_Menu_Root = gtk_menu_new();
		
		//The core name ("Core " + number)
		char IndicatorName[TEMPI_MAX_CHARS];
		snprintf(IndicatorName, TEMPI_MAX_CHARS, "TempI_Core %u", core_number);
		
		core->Gtk_Indicator_Name = strdup(IndicatorName);

		core->Gtk_Menu_Root_Description = gtk_menu_item_new_with_label("Nothing to see here!");
		gtk_menu_append(GTK_MENU(core->Gtk_Menu_Root), core->Gtk_Menu_Root_Description);
		gtk_widget_set_sensitive(GTK_WIDGET(core->Gtk_Menu_Root_Description), FALSE);
		gtk_widget_show(core->Gtk_Menu_Root_Description);
		
		core->Gtk_Indicator=app_indicator_new(core->Gtk_Indicator_Name, icon, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
		app_indicator_set_status(core->Gtk_Indicator, APP_INDICATOR_STATUS_ACTIVE);
		
		app_indicator_set_menu(core->Gtk_Indicator, GTK_MENU(core->Gtk_Menu_Root));
}

//This function calls 'sensors' to find out how many cores are being monitored
//return -1 = error
int TempI_Number_Of_Cores(){
	int NoC = 0;
	
	FILE *sensors_proc;
	char sensor_line[TEMPI_MAX_CHARS];
	
	sensors_proc = popen("sensors","r");
	D_check_pointer(sensors_proc);
	
	while(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
		if(strncmp(sensor_line, "Adapter: Virtual device", 23) == 0){
			//Ignore virtual device
			for(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc); sensor_line[0] != '\n';
				fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
				//Ignored
			}
		} else if(strncmp(sensor_line, "Adapter:", 8) == 0){
			//Count lines
			NoC = 0;
			for(fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc); sensor_line[0] != '\n';
				fgets(sensor_line, TEMPI_MAX_CHARS, sensors_proc)){
				//Ignoring information, counting cores
				NoC++;
			}
		}
	}
	
	pclose(sensors_proc);
	
	return NoC;

error:
	return -1;
}

//Callback to quit the program (Gtk style)
void TempI_Callback_Quit(GtkWidget *CallingWidget, gpointer funcdata){
	gtk_main_quit();
}

//This function sets the main indicator
void TempI_Set_Main_Indicator(TempI_Main_t *self){
	self->Gtk_Menu_Root = gtk_menu_new();
	
	self->Gtk_Menu_Root_Description = gtk_menu_item_new_with_label("About");
	gtk_menu_append(GTK_MENU(self->Gtk_Menu_Root), self->Gtk_Menu_Root_Description);
	gtk_signal_connect(GTK_OBJECT(self->Gtk_Menu_Root_Description), "activate", GTK_SIGNAL_FUNC(TempI_Show_About), self);
	gtk_widget_show(self->Gtk_Menu_Root_Description);
	
	self->Gtk_Menu_Root_Separator = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(self->Gtk_Menu_Root), self->Gtk_Menu_Root_Separator);
	gtk_widget_show(self->Gtk_Menu_Root_Separator);
	
	self->Gtk_Menu_Root_Quit = gtk_menu_item_new_with_label("Quit");
	gtk_menu_append(GTK_MENU(self->Gtk_Menu_Root), self->Gtk_Menu_Root_Quit);
	gtk_signal_connect(GTK_OBJECT(self->Gtk_Menu_Root_Quit), "activate", GTK_SIGNAL_FUNC(TempI_Callback_Quit), NULL);
	gtk_widget_show(self->Gtk_Menu_Root_Quit);
	
	self->Gtk_Indicator = app_indicator_new("TempI_Main", self->Gtk_Indicator_Icon_Path, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	app_indicator_set_status(self->Gtk_Indicator, APP_INDICATOR_STATUS_ACTIVE);

	app_indicator_set_menu(self->Gtk_Indicator, GTK_MENU(self->Gtk_Menu_Root));
}

//This function is just a clean up function to make heap memory leaks less likely
//IMPORTANT: If a memory portion was free'd, the pointer MUST be null or we will
//run into "double free" issues.
void TempI_Free_Everything(TempI_Main_t *self){
	if(self->LogFile){
		fclose(self->LogFile);
		self->LogFile = NULL;
	}
	
	for(int i=0; i < TEMPI_MAX_CORES; i++){
		if(self->Core[i].Gtk_Indicator_Name){
			free(self->Core[i].Gtk_Indicator_Name);
			self->Core[i].Gtk_Indicator_Name = NULL;
		}
	}
	
	for(int i=0; i < 4; i++){
		if(self->Gtk_Core_Icon_Path[i]){
			free(self->Gtk_Core_Icon_Path[i]);
			self->Gtk_Core_Icon_Path[i] = NULL;
		}
	}
	
	if(self->Gtk_Indicator_Icon_Path){
		free(self->Gtk_Indicator_Icon_Path);
		self->Gtk_Indicator_Icon_Path = NULL;
	}
	
	if(self->ExecutablePath){
		free(self->ExecutablePath);
		self->ExecutablePath = NULL;
	}
	
	if(self->LogFileName){
		free(self->LogFileName);
		self->LogFileName = NULL;
	}
}

//This function resolves the executable current path. It's necessary mostly for the
//libappindicator icon loading, since it doesn't seem to be able to resolve incomplete paths.
//Returns 0 if all went fine, 1 if it didn't.
char *TempI_Resolve_Executable_Path(){
	FILE *proc_maps;
	char proc_maps_line[TEMPI_MAX_CHARS]; 	//Is it better to have this variable on the stack?
											//Or should I allocate it on the heap? (overhead of asking the system for a place on
											//the heap and all the allocation routines)
	char *string_pointer;
	
	proc_maps = fopen("/proc/self/maps","r");
	D_check_pointer(proc_maps);
	
	//Only first line is necessary
	fgets(proc_maps_line, TEMPI_MAX_CHARS, proc_maps);
	fclose(proc_maps);

	//It will look something like this:
	//00400000-0040c000 r-xp 00000000 fc:01 18612248                           /bin/cat
	
	//Points to the last character, the newline
	string_pointer = strchr(proc_maps_line, '\n');
	*string_pointer = '\0'; //Replaces newline with null character
	
	//Remove application name
	string_pointer = strrchr(proc_maps_line,'/'); //Finds last occurence of '/'
	*string_pointer = '\0'; //The application name is still on the variable, but is ignored because of the null terminator
	
	//While string_pointer doesn't point the a space (meaning the path is over)
	while(*string_pointer != ' '){
		string_pointer--;
	}
	
	//Now string pointer is pointing to the first character in the path
	string_pointer++;

	//Allocates memory long enough to hold the path up to the first null terminator '\0'
	size_t path_size = strlen(string_pointer)+1;
	char *ExecutablePath = malloc(path_size);
	
	D_check_pointer(ExecutablePath);
	
	strncpy(ExecutablePath, string_pointer, path_size);
	
	D_debug("Executable Path: %s", ExecutablePath);
	
	return ExecutablePath;

error:
	return NULL;
}

//Reads configuration file and sets the program according to it
//Returns 0 if all went fine, 1 if something went wrong.
int TempI_Read_Config(TempI_Main_t *self){
	FILE *config_file;
	char *config_path;
	char config_line[TEMPI_MAX_CHARS];
	
	config_path=TempI_Concatenate_Path(self->ExecutablePath, "/Config/TempI.config");
	
	D_check_pointer(config_path);
	
	D_debug("Configuration file path: %s", config_path);
	
	config_file = fopen(config_path, "r");
	
	D_check_pointer(config_file);
	
	char *string_pointer;
	
	while(fgets(config_line, TEMPI_MAX_CHARS, config_file)){
		if(strncmp(config_line, "DELAY=", 6) == 0){
			string_pointer = strchr(config_line, '=');
			string_pointer++;
			
			self->Delay = atoi(string_pointer);
		} else if(strncmp(config_line, "LOG=", 4) == 0){
			string_pointer = strchr(config_line, '=');
			string_pointer++;
			
			if(strncmp(string_pointer, "TRUE", 4) == 0){
				self->Log = 1;
			} else {
				self->Log = 0;
			}
		} else if(strncmp(config_line, "LOGFILE=", 8) == 0){
			//Removes newline
			string_pointer = strrchr(config_line, '\n');
			*string_pointer = '\0';
			
			string_pointer = strchr(config_line, '=');
			string_pointer++;
			
			//Size of file name + 1 to store \0
			size_t FileNameSize = strlen(string_pointer)+1;
			self->LogFileName = malloc(FileNameSize);
			strncpy(self->LogFileName, string_pointer, FileNameSize);
		}
	}
	
	//Default value for delay if a improper value is given
	if(self->Delay < 1 || self->Delay > 5){
		self->Delay = 2;
	}
	
	D_debug("Logfile name: %s", self->LogFileName);
	D_debug("Delay: %u", self->Delay);
	D_debug("Log: %u", self->Log);
	
	fclose(config_file);
	free(config_path);
	
	return 0;
	
error:
	
	if(config_path){
		free(config_path);
	}
	return 1;
}

void TempI_Show_About(GtkWidget *CallingWidget, gpointer funcdata){
	TempI_Main_t *self;
	self = (TempI_Main_t *) funcdata;
	
	char *Authors[] = { "IanCaio - iancaio_dev@hotmail.com" , NULL };
	
	GtkWidget *About_Icon = gtk_image_new_from_file( self->Gtk_Indicator_Icon_Path);
	
	gtk_show_about_dialog(NULL, "program-name", "TempI", "logo", gtk_image_get_pixbuf( GTK_IMAGE(About_Icon) ),
		"title", "About TempI Indicator", "authors", Authors,
		"version", "0.1a", "website", "https://github.com/IanCaio/TempI", NULL);
}
