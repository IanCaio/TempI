#define TEMPI_MAX_CORES 4
#define TEMPI_MAX_CHARS 4096
#define TEMPI_COOL 50

//Structures
typedef struct TempI_CPU_Core_Struct{
	//General info
	int Temperature; //Current CPU Core temperature
	int High_Value; //What is the CPU Core high temperature?
	int Critical_Value; //What is the CPU Core critical temperature?
	int Status; //0 = Cool, 1 = Above Cool, 2 = Hot, 3 = Close to critical!
	
	//Gtk objects (since each Core will have a icon+label on the panel, it's easier to keep them here)
	char *Gtk_Indicator_Name; //Name used as the Gtk_Indicator ID
	AppIndicator *Gtk_Indicator;
	char Gtk_Indicator_Label[20]; //Label with the current temperature
	GtkWidget *Gtk_Menu_Root; //Menu, necessary for displaying the indicator
	GtkWidget *Gtk_Menu_Root_Description; //Just a description of the core being monitored (and exact temperature)
} TempI_CPU_Core;

typedef struct TempI_Main_Struct{
	//General use variables
	int Delay; //Delay between each 'sensors' process call
	int Log; //Should we write to a log file?
	char *LogFileName; //The log file name
	FILE *LogFile; //Log file handler
	char *ExecutablePath; //Executable full path
	
	//CPU Cores
	TempI_CPU_Core Core[TEMPI_MAX_CORES];
	int Cores_Counter; //How many cores are being monitored?
	char *Gtk_Core_Icon_Path[4]; //Path to different icons for the CPU indicator (there are 4 icons).
	
	//Gtk objects
	char *Gtk_Indicator_Icon_Path; //Path to main indicator icon
	AppIndicator *Gtk_Indicator;
	GtkWidget *Gtk_Menu_Root;
	GtkWidget *Gtk_Menu_Root_Description;
	GtkWidget *Gtk_Menu_Root_Separator;
	GtkWidget *Gtk_Menu_Root_Quit;
} TempI_Main_t;

//Function Prototypes
void TempI_Free_Everything(TempI_Main_t *self); //Frees memory
char *TempI_Resolve_Executable_Path(); //Resolves executable path and return pointer to the string containing it
int TempI_Resolve_Icons_Path(TempI_Main_t *self); //Resolves path to the icons for the core indicators
int TempI_Read_Config(TempI_Main_t *self); //Reads config file and sets the variables on TempI_Main
void TempI_Set_Main_Indicator(TempI_Main_t *self); //Sets the main gtk app indicator
void TempI_Set_Core_Indicator(TempI_CPU_Core *core, char *icon, int id); //Sets the cores gtk app indicators
void TempI_Callback_Quit(GtkWidget *CallingWidget, gpointer funcdata); //Callback to quit program
int TempI_Number_Of_Cores(); //Returns the number of cores
gint TempI_Update(gpointer datapointer); //Updates the indicator
int TempI_Get_Core_Temperatures(TempI_Main_t *self); //Updates the temperatures values on every core
char *TempI_Concatenate_Path(const char *root, const char *subdir); //Adds the subdirectory to the root directory and
																	//returns a malloc string with the result of the concatenation
void TempI_Show_About(GtkWidget *CallingWidget, gpointer funcdata); //Shows the about dialog
