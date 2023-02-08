const char *ValidCommand = "The command you have typed is not valid! Type 'help' to see all the available commands.";
const char *TakenUsername = "Sign-up Error! Username is already taken";
const char *Success = "Success";
const char *AlreadyLogged = "You can't perform this action when logged in!";
const char *FieldNumber = "Invalid Data. Please complete all fields [don't use spaces]!";
const char *InvalidCredentials = "Login Error! Username/Password incorrect. Please Try Again!";
const char *BindErr = "Error at bind()";
const char *SocketErr = "Error at socket(): ";
const char *AcceptErr = "Error at accept(): ";
const char *ListenErr = "Error at listen(): ";
const char *AppCrash = "An error happened and the application crashed.\n";
const char *ConnectErr = "Error at connect()";
const char *UsernameErr = "Invalid Username!\nUsername should contain minimum 5 characters and shouldn't contain spaces.\n";
const char *PasswordErr = "Invalid Password!\nPassword should contain minimum 6 characters and shouldn't contain spaces.\n";
const char *SubErr = "Invalid Response !\nPlease type either y or n !\n ";
const char *AuthSuccess = "Authentification Done Successfully!";
const char *InvalidSportOpt = "The second argument should be an option between [0, 1, 2]!";
const char *ShouldLog = "You should be logged in to complete this command!";
const char *SportSubscribe = "You should be subscribed to the sports channel to complete this command!";
const char *NoDate = "The argument you have typed is not a valid date [dd-mm-yyyy]!";
const char *WeatherSubscribe = "You should be subscribed to the weather channel to complete this command!";
const char *PecoSubscribe = "You should be subscribed to the peco channel to complete this command!";
const char *AlreadyPecoSub = "You are already subscribed to the peco channel!";
const char *AlreadySportSub = "You are already subscribed to the sports channel!";
const char* AlreadyWeatherSub = "You are already subscribed to the weather channel!";
const char *UnsubMessage = "Your unsubscription request has been processed. We are sad to see you go.";
const char *SubMessage = "Your subscription request has been accepted. Welcome to the channel!";
const char *UnknownStreet = "The street you are currently on is not find in our database.";
const char *NoNotif = "The inbox is empty."; 



const char *Help = "Faze is an application that consists of three, very important parts:\n" 
"1. A tracker that is constantly telling your current location and traveling speed. "
"If you are surpassing the speed limit, the message will be coloured red, otherwise it will be green.\n" 
"2. A notification thread that is constantly showing you details about the most recent"
" event reported by any user. These messages will be coloured in yellow.\n" 
"3. A thread that constantly listens for some specific commands typed by the user. A command should be typed after the -> symbol and should be one of the following:\n"
"3.1. sign-up - The user might create an account on the application." 
"This command will be followed by a prompt, asking for details such as the first name, last name, the desired username, desired password, and whether or not the user wants to subscribe to channels such as sports news, weather condition, and pecos information.\n"
"3.2. login username - Logs the user into the application. This command is followed by a password prompt.\n"
"3.3. report evcode - Reports an event codified by evcode that is taking place on the street you are currently located. The events are codified as follows:\n"
        "evcode = 0 -> Accident\n"
        "evcode = 1 -> Traffic Jam\n"
        "evcode = 2 -> Road Repair\n"
        "evcode = 3 -> Police Patrol\n"
"3.4. get-police-info - Prints information about every police patrol reported by other users.\n"
"3.5. get-accidents-info - Prints information about every accident that happened in the last hour.\n"
"3.6. get-repair-info - Prints information about every road that is being repaired.\n"
"3.7. get-jam-info - Prints information about every traffic jam recorded in the last hour.\n"
"3.8. get-sports-info option - For users who are subscribed to the sports channel, it prints important news about a sport specified by the option argument as follows:\n"
        "option missing -> All Sports\n" 
        "option = 1 -> Football\n"
        "option = 2 -> Basketball\n" 
        "option = 3 -> Tennis\n"
"3.9. get-weather-info [date] - For users who are subscribed to the weather channel, it prints information about each date in the database. Optionally, " 
"a date argument might be provided to get the weather on a specified date.\n"
"3.10. get-peco-info [name] - For users who are subscribed to the peco channel, it prints information about all available filling stations. Optionally a name argument might be provided "
"to filter the filling stations available by name.\n"
"3.11. subscribe-peco - Subscribe to the peco channel.\n"
"3.12. subscribe-weather - Subscribe to the weather channel.\n"
"3.13. subscribe-sports - Subscribe to the sports channel.\n"
"3.14. unsubscribe-peco - Terminate subscription to the peco channel.\n"
"3.15. unsubscribe-weather - Terminate subscription to the weather channel.\n"
"3.16. unsubscribe-sports - Terminate subscription to the sports channel.\n"
"3.17. logout - log out the user from the application.\n"
"3.18. quit - close the application";

