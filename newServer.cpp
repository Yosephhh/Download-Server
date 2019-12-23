/********************************************************************************************************
 * Author: Yoseph Mamo
 * Creation Date: November 20, 2018
 * Filename: newServer.cpp
 * Purpose: This is a server side of a download server application
 * Programming Language Used: C++
 * Compiler Used: clang++ 3.4.2 
 * Compiler command: clang++ -std=c++11 newServer.cpp
 * Execution command: ./a.out for default port Number or
                      ./a.out <PORTNUMBER> for user specified port Number
                      use ./a.out <PORTNUMBER>  2 > (fileName) To see the debug messages saved in a file
 * Protocol: ->  All Messages between client and server must be terminated by ':)' Character sequence.
             ->  if the above condition fails then  program exits, 
	         ->  Possible Message/Command from client "bye:)"
 *
 *********************************************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h> // for socket
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h> // perror
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h> // inet_addr
#include <netdb.h> // gethostbyname
#include <iostream> 
#include <string.h>
#include <string> // For String
#include <fstream> // For file check
#include<sys/wait.h> // for wait
using namespace std;

#define DEFAULT_PORT 49878
#define MAX_MSG_SIZE 5000
void usageClause(const char *argv[]);
bool isNumeric(const string str);
void checkReply( char* clientReply, const int connectedSock, const int listeningSock,  string ipAddress);
bool hasEndOfMsg (const string str);
void connectToClient(int &sockfd, int &client_socket, sockaddr_in  &address);
string getIpAddress(sockaddr_in &address);
void sendToClient(const int sockfd, const char* messsage, bool printToScreen);
void recvFromClient(const int sockfd, char clientReply[]);
bool doesFileExist(const char *file);
void sendDirListing(int connectedSock);
void runServer(int &sockfd, int &client_socket, sockaddr_in  &address, char clientReply[]);

/********************************************************************************************************************************
 * Function name:     main
 * Description:       
 * Parameters:        int argc: the number of command line arguments passed
                      const char *argv[]: array of command line arguments passed, s
 * Return Value:      exit status of the program

********************************************************************************************************************************/

int main(int argc, char const  *argv[]) 
{ 
  int sockfd = 0;
  int client_socket = 0; // For Socket once Connected to client 

  
  struct sockaddr_in address;
  address.sin_family = AF_INET; // Specify the Address Family
  address.sin_addr.s_addr = INADDR_ANY; //Specify The IP Addresses

  // Few Arguments passed
  if(argc < 2)
    {
      // Assign Default Port
      address.sin_port = htons( DEFAULT_PORT ); 
    }
  // Too many arguments passed 
  else if(argc > 2)
    {
      usageClause(argv);
    }
  // right amount of arguments passed
  else if (argc == 2)
    { 
      
      // Check if port number entered is Numeric
      if(!isNumeric(argv[1]))
	{
	  cout << "Optional Port Number Must be all Numeric !" << endl;
	  usageClause(argv);	  
	}
      else 
	{
	  // Convert the command line argument to a number 
	  string input = argv[1];
	  // Convert the string to a number
	  int optionalPort = atoi(input.c_str());
	  // Check if the port number entered is between the range 
	  if( !(optionalPort > 1024 && optionalPort < 65535) )
	    usageClause(argv);
	  else
	    cout << "\nOptional Port # Specified By The Server: " << input << endl;
	  // Set the port number specified by the client 
	  address.sin_port = htons( optionalPort );
	}
    }
  
  
  char clientReply[MAX_MSG_SIZE] = {'\0'}; // To Store reply message from the client  
  connectToClient(sockfd, client_socket,  address);
  
  int addrlen = sizeof(address);
  int numChild = 0;	
  // Infinite loop
  while(true)
    {
      // Accept incoming connections form client
      if ((client_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
	{ 
	  perror("accept"); 
	  exit(EXIT_FAILURE); 
	}    
      int rv = fork(); // Fork()
      switch(rv)
	{
	case -1: // fork() error
	  {
	    perror("Fork() Failed !");
	    exit(-1);
	  }
	case 0: //Child
	  {
	    runServer(sockfd,client_socket,address, clientReply);
	  }
	default: // Parent
	  {
	    numChild++; // Keep Track of the number of child processes.
	  }
	}// end switch
      
    }// end while
  
  for(int i =0; i < numChild; i++)
    {
      int stat;
      pid_t  childPid; //To store wait return value: wait() returns process id of terminated child (if there is one)
      childPid = wait(&stat); // using wait() to suspend Parent process until its child terminates
      if(childPid == -1) // No child to wait for :(
	{
          perror("Error in Wait!");
          continue;
        }
      // Output information about the status of the child process terminated
      if(WIFEXITED(stat))// WIFEXITED(stat) will return true if the child terminates
        {
          // WEXITSTATUS(stat) will return the exit status of the child
          cout << "Child: " << childPid << " is terminated: Return Status: " << WEXITSTATUS(stat) << endl;
        }
      else
        cout << "Child: " << childPid << " is terminated, return status is unknown. "  << endl;
      
      
      numChild--;
      
    }
  close(sockfd); // Close the listening socket
  
  
}// end main()
void runServer(int &sockfd, int &client_socket, sockaddr_in  &address, char clientReply[])
{
  const char *hello = "Hello Client. "; // Servers  Hello Message For the Client 
  // Get the Ip Address of the connected Socket 
  string ipAddress = getIpAddress(address);  
  // Send Hello Message to Client
  sendToClient(client_socket, hello, true);
  // Receive Message (Command) From Client
  recvFromClient(client_socket,clientReply);
  // convert to string for == comparison 
  string clientMessage(clientReply);
  
  while(clientMessage != "bye")
    {       
      // Check if the client wants to exit and end the connection.
      checkReply(clientReply, client_socket,sockfd,  ipAddress);
      // Receive Message (Command) From Client
      recvFromClient(client_socket,clientReply);	  
    }
  
}
/**********************************************************************************************
 * Function name:     sendToClient
 * Description:       Send Message To Client, Handle Errors Appropriately
 * Parameters:        const int sockfd: Socket Descriptor of connected Socket
                      const char* message: A message to be sent to the client
		      * Return Value:       void (none)
		      *********************************************************************************************/
void sendToClient(const int sockfd, const char* message, bool printToScreen)
{
  string userMsg = message; // store the message that will be outputted to the screen
  // remove the end of message marker (for user output)  
  const char* clientMsg = (userMsg + ":)").c_str();// store the message that will be sent to the client 
  
  if(send(sockfd, clientMsg, strlen(clientMsg), 0 ) < 0 ) // For char
    {
      perror("Sending Failed ! ");
      exit(-1);
    }
  else
    if(printToScreen)
      cout << "Message Sent: \"" <<  userMsg << "\"" << endl;
  
} 
/**********************************************************************************************
 * Function name:     recvFromClient
 * Description:       Receive Message From Client, Handle Errors Appropriately
 * Parameters:        const int sockfd: Socket Descriptor of connected Socket
                      char clientReply: A char array to store the message received from client
		      * Return Value:      void (none)
 *********************************************************************************************/
void recvFromClient(const int sockfd, char clientReply[])
{
  
  memset(clientReply, 0 , MAX_MSG_SIZE);
  if(recv(sockfd, clientReply, MAX_MSG_SIZE, 0) < 0) // Error In Receive
    {
      perror("Recieving Failed ! ");
      exit(-1);
    }
  else// recv success
    {
      while(!hasEndOfMsg(clientReply))// Check if the message from the client has (end of message character) 
	{
	  if(recv(sockfd, clientReply, MAX_MSG_SIZE, 0) < 0) // Error In Receive
	    {
	      perror("Recieving Failed ! ");
	      exit(-1);
	    }	
	}// end while
      
      // Since this gets executed when the client send a complete message (a message with end of file character)
      // Remove the end of message character for every receive here (instead of later checking for if("pwd:)" 
	  // just check if(pwd)
      clientReply[strlen(clientReply) -2] = '\0'; 
	  cout << "\nMessage from The Client : \"" << clientReply << "\"" << endl;
    }
  /*
    piazza
     else
     {
     // Inform The User the Connection Has Ended
     cout << "Failed Client Message: " << clientReply << endl;
     cout << "Message Not Fully Received From Client." << endl;	 
	  exit(-1);
	  }
  */  
  
}// end revcFromClient

/********************************************************************************
 * Function name:     usageClause
 * Description:       Output the usage Clause for the user
 * Parameters:        char *argv[]: array of char pointers that contain the  arguments
 * Return Value:      void (none)
 *********************************************************************************/
void usageClause(const char *argv[])
{
  
  cout << "PORT NUMBER Must be between the range 1024 and 65535 " << endl;
  cout << "\nUsage: " << argv[0] << " <PORT NUMBER > \n" << endl;
  exit (-1);
}//end usageClause()
/*******************************************************************************************************
 * Function name:     isNumeric
 * Description:       Checks if a string of characters is all Numeric or not 
 * Parameters:        const string str: A string that is going to be checked for all numeric or not
 * Return Value:      true:  if the string is all numeric(if its a number)
                      false: if the string is not all numeric (if it's not a number)
*******************************************************************************************************/
bool isNumeric(const string str)
{
  // loop Through each character in the string
  for(char x:  str)
    if(!isdigit(x)) // Check if a single character "x" its a digit
      return false;  // if its not return false
  
  return true; // else return true
}// end isNumeric
/********************************************************************************************************************************
 * Function name:     hasEndOfMsg
 * Description:       Checks if a string of characters contains the end of indicator specified in the protocol, in this case ":)"
 * Parameters:        const string str: A string that is going to be checked for a sequence of specified characters
 * Return Value:      true:  if the string contains the character sequence of ":" followed by ")" which is ":)"
                      false: if the string fails to satisfy the above condition

********************************************************************************************************************************/
bool hasEndOfMsg(const string str)
{
  
  int i = 0;
  while(i < str.length()) // Not To Go Out Of Bounds while checking for i and i+1
    {
      // loop Through each character in the string
      for(i =0; i <str.length(); i++)
	if(str[i] == ':' && str[i+1] == ')')// If there is a :) sequence of characters
	  return true;
    }

  return false;
}// end hasEndOfMsg
/********************************************************************************************************************************
 * Function name:     checkReply
 * Description:       Checks the client reply  (message/command) for the download protocol, and runs commands apprpriately 
 * Parameters:        const char* clientReply: The message recieved form the client 
                      const int connectedSock: The socket descriptor for the connected socket (The Client Socket)
                      const char* ipAddress: The Ip Address of the connected socket 
* Return Value:     void(none)
********************************************************************************************************************************/
void checkReply( char* clientReply, const int connectedSock, const int listeningSock,  string ipAddress)
{

  int messageSize = strlen(clientReply);
  // Good Bye Message For The Client
  const char *goodByeMessage = "Good Bye Client.";
  // Convert Message from client to string, useful Later  to use == comparison
  string clientMessage(clientReply);
  
  if(clientMessage == "bye")
    {
      // Send A Good Bye Message      
      sendToClient(connectedSock, goodByeMessage, true);
      // Close Connection
      if(close(connectedSock) == -1)
	{
	  perror("Error Closing Connected Socket: ");
	}     
      // Inform The User the Connection Has Ended
      cout << "Connection With: " << ipAddress << " Has Ended !" << endl;
      
      exit(-1);
    }
  else if(clientMessage == "pwd")
    {
      
      // Allocate a Buffer to store the current working directory name 
      char directory[MAX_MSG_SIZE];
      
      // Get the Current Working Directory by the getcwd() function call also check for errors
      if((getcwd(directory , sizeof(directory) ) == NULL ) )
	{
	  perror("Couldn't Get Current Working Directory");
	}
      // Convert to directory string
      string strDirectory = directory;
      // Send the working Directory to the Client 
      sendToClient(connectedSock, strDirectory.c_str(), true);
      
    }
  else if(clientMessage == "cd") // Change Directory
    {
      
      const char *reqDirMsg = "Enter the New Directory: "; 
      // memset(clientReply, 0 , sizeof clientReply);
      char newDirectory[MAX_MSG_SIZE]; // for new directory
      
      // Prompt the client For the directory name(to change to)
      sendToClient(connectedSock, reqDirMsg, true);
      // Read the new directory to change to
      recvFromClient(connectedSock, newDirectory);
      
      // If directory changing fails
      if(chdir(newDirectory) == -1)
	{
	  // An error message from the server to the client
	  char errorMsg[MAX_MSG_SIZE] = "Couldn't change to specified directory: ";
	  // Append the error specified by the system call 
	  const char* errorMessage  = strcat(errorMsg, strerror(errno));
	  perror("Couldn't Change to New Directory");
	  // send a combined  error message to client
	  sendToClient(connectedSock,errorMessage,true); 
	}// end if
      else
	{
	  string successMsg = "Directory has Successfully Changed to: ";
	  successMsg+=newDirectory;
	  sendToClient(connectedSock, successMsg.c_str(),true);
      
	}// end else
      
      
    }// end else if
  else if (clientMessage == "download")
    {
      struct stat val; // statStr;	
      ifstream ins;
      char fileName[MAX_MSG_SIZE];
      char responce[MAX_MSG_SIZE];
      const char *prompt = "Enter the File Name: "; // Ask Client For The File Name
      const char *readyMessage = "READY"; // A ready Message if file was found
     
      // Prompt the client for the file name 
      sendToClient(connectedSock, prompt, true);
      // Receive File Name From Server;
      recvFromClient(connectedSock, fileName);
      
	  stat(fileName, &val);	
	  string strFileName = fileName;
	  
		
	  if(doesFileExist(strFileName.c_str()) == true)// If the File Exists 
	    {
	      ///if(!S_ISDIR(val.st_mode)) // If the fileName  is not a directory
	      if ((val.st_mode & S_IFMT) == S_IFREG)
		{
			  // Send Ready message For Client 
		  sendToClient(connectedSock, readyMessage, true);
		  // receive "Ready" Or " Stop from client
		  recvFromClient(connectedSock, responce);
		  // convert clients response for string comparison
		  string strResponce = responce;
		  cout << "Client : " << responce << endl;
		  
		  if(strResponce == "READY")
		    {	      
		      //string file1 = "data.txt";
		      ins.open(strFileName.c_str());	      
		      // Var to store file, initially Empty
		      string file = "";
		      // Var to store line read from a file
		      string line  ="";
		      
		      while(!ins.eof())
			{
			  // Get the first line from the file and store it inside line
			  getline(ins,line);
			  // Add the line to the file 
			  file+=line;
			  // Append a new line to the file after each line is read
			  file +="\n";
			} // once file completely read
		      
		      // Send File to Client
		      sendToClient(connectedSock, file.c_str(), false);
		      // Receive message? did client get complete file?
		      recvFromClient(connectedSock, responce);	      
		    }
		  else if(strResponce == "STOP") // Client Doesn't Want File To Be Downloaded Anymore
		    {
		      const char *stop = "Download Canceled.";
		      sendToClient(connectedSock, stop, true);
		    }
		  
		  //Close File 
		  ins.close();
		}
	      
	      else
		{  
		  string strErrorMsg = "Download Failed: " + strFileName + " is a directory not a file! ";		  
		  // send a combined  error message to client
		  sendToClient(connectedSock, strErrorMsg.c_str(), true);
		}
	    }//if(!S_ISDIR(val.st_mode))
	  else
	    {  
	      // An error message from the server to the client
	      char errorMsg[MAX_MSG_SIZE] = "Download failed: "; 
	      // Append the error specified by the system call
	      const char* errorMessage  = strcat(errorMsg, strerror(errno));
	      // send a combined  error message to client
	      sendToClient(connectedSock, errorMessage, true);
	    }
    }
  else if(clientMessage == "dir")
    {
      // Send Directory Listing to client (error handled inside function)
      sendDirListing(connectedSock);
    }
  /*
    else
    {
    cout << "Invalid Entry For the Server " << endl;
    close (connectedSock);
    exit(-1);
    }
  */
  
}// end checkReply
/********************************************************************************************************************************
 * Function name:     connectToClient
 * Description:       Establishes a connection to a client using the system calls socket, bind, listen, and accept. Will display an error message if  
 * Parameters:        const char* clientReply: The message received form the client
                      int &connectedSock: The socket descriptor for the connected socket (The Client Socket)
                      const char* ipAddress: The Ip Address of the connected socket
		      * Return Value:     void(none)

		      ********************************************************************************************************************************/
void connectToClient(int &sockfd, int &client_socket, sockaddr_in  &address)
{
  int addrlen = sizeof(address);
  
 // Create Socket, check for success/error
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
      perror("socket failed"); 
      exit(EXIT_FAILURE); 
    } 
  // attaching socket to the DEFAULT_PORT
  if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
      perror("bind failed"); 
      exit(EXIT_FAILURE); 
    } 
  // SOMAXCONN special constant to specify the length of the queue of pending connections Maximum reasonable value. 
  if (listen(sockfd, SOMAXCONN) < 0) 
    { 
      perror("listen"); 
      exit(EXIT_FAILURE); 
    }

  /*  // Accept the client
      if ((client_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
    perror("accept");
    exit(EXIT_FAILURE);
    }
  */
  
}
/*************************************************************************************************************************************
 * Function name:     getIpAddress
 * Description:       Converts the IPv4 and IPv6 addresses from binary to text form
 * Parameters:        struct sockaddr_in &address: A sockaddr_in structure used to store the addresses for the internet address family 
 * Return Value:      void(none)

 ************************************************************************************************************************************/

string getIpAddress(sockaddr_in &address)
{
  
  // To store the IP address of the client 
  char ipAddress[50];

  ///  Get the ip Address of the Client  
  if(inet_ntop(AF_INET,&(&address)->sin_addr, ipAddress, sizeof ipAddress ) == NULL )
    {
      perror("Couldnt Get Clients IP Address !");
    }
  // Output The Ip address the server is connected to (Clients ip)
  cout << "\nConnected To: " << ipAddress << endl;
  
  // Convert the Ip Address to a string 
  string strIpAddress = ipAddress;

  return strIpAddress;
  
}

/******************************************************************************************************************* \
 ******************
 * Function name:     doesFileExist
 * Description:       Checks if a file exists or not
 * Parameters:        none
 * Return Value:      bool : (True if file exists, false if not)

*******************************************************************************************************************\
*****************/
bool doesFileExist(const char *file)
{
  ifstream ins;
  ins.open(file);
  return ins.good();
}

void sendDirListing(int connectedSock)
{

  struct dirent *dirStrPtr;    // pointer to directory structure
  DIR *directoryPtr;           // current directory pointer
  char *dirname=(char *)calloc(80, sizeof(char));    // allocate memory and set it to 0s
  char *filename=(char *)calloc(256, sizeof(char));  // allocate memory and set it to 0s
  char *errmsg=(char *)calloc(256, sizeof(char));    // allocate memory and set it to 0s
  struct stat statStr;         // stat structure
  string header = "\nFiles are  Marked With ** \n\n";
  string dirList = header;
  /* Use current directory */
  strcpy(dirname, ".");
  
  /* Open Directory */
  if ((directoryPtr = opendir(dirname)) == NULL)   {
    perror("Cannot open current directory: ");
    exit(2);
  }   /* end if */
  

  /* set errno to 0 so can check to see if the system call sets it for an error */
  errno = 0;

  /* While there are still contents in the directory to read */
  while ((dirStrPtr = readdir(directoryPtr)) != NULL)   {
    /* Get the status of the current entry */
    sprintf(filename, "%s/%s", dirname, dirStrPtr->d_name);
    if (stat(filename, &statStr) == -1)  {
      sprintf(errmsg, "Error stat(%s): ", filename);
      perror(errmsg);
      continue;
    }  // end if stat

    /* Save entry name; mark files with an * */
    if ((statStr.st_mode & S_IFMT) == S_IFREG)
      {
	dirList += dirStrPtr->d_name; // Add to the directory list
	dirList += "  **"; // append ** if its a file
	dirList += "\n"; // Append New Line
      }
    else
      {
	
	dirList += dirStrPtr->d_name;// Add to the directory list
	dirList += "\n";// Append New Line
      }
    
    // reset errno to 0
    errno = 0;
  }   /* end while */
  
  /* Check for an error */
  /* If there is an error, NULL will be returned and errno will be set */
  if ((dirStrPtr == NULL) && (errno != 0))  {
    perror("Error reading directory entry: ");
  }  // end if error

  //  dirList += ":)"; // Append End of message char
  sendToClient(connectedSock, dirList.c_str(), false); // Send  the list to client
  
  closedir(directoryPtr);

}
