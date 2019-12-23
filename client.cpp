/********************************************************************************/
/* Author: Yoseph Mamo 															*/
/* Creation Date: November 12, 2018 											*/
/* Filename: client.cpp 														*/
/* Purpose:  client side program to test server 								*/
/* Language: C++ 																*/
/* Compiler version: clang 3.4.2  												*/
/* Compile Command: clang++ -std=c++11 client.cpp 								*/
/* Execute Command: ./a.out <Hostname> Optional: <Port Number> 2 > errors.out  	*/
/*                 Do 2 > errors.out if you would like 							*/
/*                 to see meesages sent to server 								*/ 
/* Protocol: All messages that are sent to the server will end with ':)' 		*/
/*          																	*/
/********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <fstream>
#include <iomanip>

#define DEFAULT_PORT 49878 // Default port to connect to server
#define MAX_MSG_SIZE 5000 // Max size of message 

//Function Prototypes
bool isNumeric(const std::string str);//Helper function to determine if string is numeric
bool isEndOfMsg(const std::string str); // Helper function to determine if EOM sequence is in message
void sendToServer(const int sockfd, const char* message); // Send message to server
void recvFromServer(const int sockfd, char server_reply[], bool printMsg); // receive message from server
void displayMenu(); //display menu options
void valInput(std::string input, const int sockfd, char server_reply[]); // validate input to server
std::string modifyInput(std::string input); // Helper function modify input to lowercase


/************************************************************************/
/* Function name: main */
/* Description: Send & receive messages from a server                  */
/* Parameters: int argc-  the count of command line arguments          */
/*             char **argv- a character array of with the values passed */
/*                          in from the command line                    */
/* Return Value: Exit status of program                                  */
/*************************************************************************/
int main(int argc, char **argv)
{
  int sockfd; //socket descriptor
  struct sockaddr_in servaddr;  //pointer to server info
  struct hostent *hostEnt; //pointer to host 
  struct in_addr *IPaddr; //pointer to IP address
  const char *byeMessage = "bye:)";
  
  
  //Valadating command line arguments 
  if( argc == 2) //Set Port number to default port
    {
      servaddr.sin_port = htons(DEFAULT_PORT);
    }
  else if( argc == 3)// Optional Port being used
    {
      std::string input = argv[2];
      int optionalPort = atoi(input.c_str()); //Convert port number to an integer
      
      if(!isNumeric(argv[2])) // Make sure only numbers were specified in the port number
	{
	  std::cout<< "Optional Port Number must be all numeric " << std::endl;
	  exit(-1);
	}
      if( optionalPort < 1024 || optionalPort > 65535) // Port number must be in these bounds
	{
	  std::cout<< "Optional Port must be between 1025 & 65535 " << std::endl;
	  exit(-1);
	}
      
      servaddr.sin_port = htons(optionalPort); // Set optional port number to be server port number
    }
  
  if (argc < 2 || argc > 3 ) // Only allow two or 3 Command line arguments
    {
      std::cout << argv [0] << " usage: <Hostname> Optional <Port Number>" << std::endl;
      exit(1);
    }
  
  hostEnt = gethostbyname(argv[1]); // get IP from host name
  if(hostEnt == NULL) // test if invaild host name
    {
      herror("Invalid Hostname") ;
      exit(-1);
    } // end if
  
  IPaddr= (struct in_addr *)hostEnt->h_addr; //store IP address into IPaddr
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) //create a socket
    { 
      perror("Error creating socket " ) ;
      exit(-1);
    } // end if 
  
  //specify the address Family and  Ip Address
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr= *IPaddr;
  
  //connecting to server
  if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1 )
    {  // error
      perror("Error connecting to server ");
      exit(-1);
    }//end if   
  
  
  char ipAddress[50] = {'\0'}; // Create char array for IP address to be stored
  
  //Convert IP address from binary form to Text form 
  if(inet_ntop(AF_INET,&(&servaddr)->sin_addr, ipAddress, sizeof ipAddress)==NULL) 
    {
      perror("Error getting IP ");
    } //end if
  std::cout<< "Successfully connected to " << ipAddress <<  std::endl;
  
  
  char server_reply[MAX_MSG_SIZE] = {'\0'}; // Declare buffer for message
  
  //recieving hello message from server
  recvFromServer(sockfd, server_reply, 1);
  
  std::string command;
  displayMenu();
  std::cout << "Enter Menu Option: " << std::endl;
  
  //Loop until user is done entering commands
  while(command!= "bye")
    {
      std::cout << "Command: " ;
      std::cin >> command;
      command=modifyInput(command);
      valInput(command,sockfd, server_reply); //Check command being used
      
      if(command!= "bye") //If client wants to disconnect do not display menu
	displayMenu();
      
    } // end while loop
  
  //close the connection 
  if(close(sockfd) == -1 )
    {
      perror("Error closing socket: " ) ;
      exit(-1);
    } //end if 
  std::cout<< "Connection to " << ipAddress << " closed" << std::endl; 
  
  
}
/************************************************************************/
/* Function name: isNumeric                                              */
/* Description: Determine if string that was entered contains numeric    */
/*              numbers, Used to validate port number to ensure port     */
/*              number entered is all numeric                            */
/* Parameters: const std::string str- String that was entered by user    */
/* Return Value: True- If string entered contains all numbers            */
/*               False- If string entered conatins anything that is      */
/*		 not a number                                            */
/*************************************************************************/

bool isNumeric(const std::string str)
{
  for(char x: str) // Loop each character in the string 
    {
      if ( !isdigit(x)) //Check if each character is a number
	return false; // if not a number
    }//end for loop
  return true; // if all characters are numbers 
} //End isNumeric

/************************************************************************/
/* Function name: isEndOfMsg                                             */
/* Description: Determine if string that was entered contains the end of */
/*              message sequence defined in protocol                    */
/* Parameters: const std::string str- String that was entered by user    */
/* Return Value: True- If string entered contains end of message sequence*/
/*               False- If string entered does not contain end of message */
/*                      sequence      */
/*************************************************************************/

bool isEndOfMsg(const std::string str) 
{
  int i = 0;
  while(i < str.length()) // Make sure no out of bounds errors occur
    {
      for(i=0; i <str.length(); i++) // Check for end of message sequence
	if(str[i] == ':' && str[i+1] == ')')
	  return true; // If whole message is received 
    }//end while
  return false; // End Of Message sequence is not in message 
} //end isEndOfMsg

/************************************************************************/
/* Function name: sendToServer                                           */
/* Description: Send messages to a connected Server                      */
/* Parameters: const int sockfd- socket file descriptor  */
/* const char* message - Message being sent to server    */
/* Return Value: Nothing */
/*************************************************************************/

void sendToServer(const int sockfd, const char* message)
{
  std::string userMsg = message;
  
  const char* serverMessage = (userMsg + ":)").c_str();
  if(send(sockfd, serverMessage, strlen(serverMessage), 0)== -1) // Send message to server check for failure
    {
      perror("Error sending message: " ) ;
      exit(-1);
    }
  std::cerr << "Message sent to server: \"" << userMsg << "\"" << std::endl; 
}//end sendToServer

/************************************************************************/
/* Function name: recvFromServer                                        */
/* Description: receive messages from a connected Server                */
/* Parameters: const int sockfd- socket file descriptor    */
/*             char server_reply[] - char array message received */
/*             bool printMsg- Determine if the message from server */
/*                            should be printed(Dont print contents of */
/*                            file downloaded */
/* Return Value: Nothing */
/*************************************************************************/

void recvFromServer(const int sockfd, char server_reply[],bool printMsg)
{
  
  memset(server_reply, 0, MAX_MSG_SIZE ); // Resetting buffer to be empty to receive full message
  
  if (recv(sockfd, server_reply,  MAX_MSG_SIZE, 0) == -1) // receive message check for failure
    {
      perror("Error receiving message: " ) ;
      exit(-1);      
    }//end if 
  else 
    {// Check to make sure full message was received
      while(!isEndOfMsg(server_reply))
	{ //If message was not received recv again until message is fully received
	  if (recv(sockfd, server_reply,  MAX_MSG_SIZE, 0) == -1) // receive message check for failure
	    {
	      perror("Error receiving message: " ) ;
	      exit(-1);
	    }//end if
	}
      // Since Message Successfully received, trim the end of message marker
      server_reply[strlen(server_reply) -2] = '\0';
      if(printMsg)
	std::cout<< "Message from server: \"" << server_reply << "\"" << std::endl;
      
    }//end else  
  
}// end recvFromServer

/************************************************************************/
/* Function name: displayMenu                                        */
/* Description: Display menu options for user to enter                 */
/* Parameters: None       */
/* Return Value: Nothing */
/*************************************************************************/
void displayMenu()
{// Display menu options to user
  //formatting for menu
  std::cout<< "******************************************************************************" << std::endl;
  std::cout<< "*\t\t\t\tMenu Options:" << std::setw(33) <<"*"
           << std::endl << "*" <<  std::setw(77) <<   "*" << std::endl ;
  
  std::cout << "*\tPWD - Prints working directory on server" << std::setw(30) << "*" << std::endl;
  std::cout << "*\tDIR - Prints each file name in current directory on server"
            << std::setw(12) << "*" << std::endl;
  
  std::cout << "*\tCD <Directory Name>  - Changes Directory to directory specified"
            << std::setw(7) << "*" << std::endl;
  
  std::cout << "*\tDownload <fileName> - Download specified file" << std::setw(25) << "*" << std::endl;
  std::cout << "*\tBye - Disconnect from server" << std::setw(42) << "*" << std::endl
            <<  "*" <<  std::setw(77) <<   "*" << std::endl ;
  
  std::cout<< "******************************************************************************" << std::endl;
} //end displayMenu

/************************************************************************/
/* Function name: valInput                                        */
/* Description: Validate user input to make sure command entered was   */
/*              then send the command to the server and receive reply  */ 
/* Parameters: const int sockfd- socket file descriptor    */
/*             char server_reply[] - char array message received */
/*             string input- Input from user                           */
/* Return Value: Nothing */
/*************************************************************************/
void valInput(std::string input, const int sockfd, char server_reply[])
{ // Check command user enters
  
  if (input == "pwd")
    { 
      const char* pwd="pwd"; //message being sent to server
      sendToServer(sockfd, pwd);
      recvFromServer(sockfd, server_reply,1); //receive the working directory 
    } //end if 
  
  else if (input == "dir")
    {
      const char* dir="dir"; // Message being sent to server
      sendToServer(sockfd, dir);
      recvFromServer(sockfd, server_reply,1); //receive the listing of Directories
    } // end else if
  
  else if (input == "cd")
    {
      
      const char* cd = "cd" ; //message being sent to server
      std::string dirName; //Directory name
      std::cin >> dirName; //Getting directory that wants to be changed to  
      
      const char * newDir = dirName.c_str(); //convert dirName to C string to send to server
      
      sendToServer(sockfd, cd); //Sending the command first to server
      recvFromServer(sockfd, server_reply,1); //Receiving which directory user wants to send to
      
      sendToServer(sockfd, newDir); // Sending the directory to change to
      recvFromServer(sockfd, server_reply,1); //Receiving success or fail message
    } // end else if 
  
  else if (input == "download")
    {
      //messages to be sent to server 
      const char* ready = "READY";
      const char* stop = "STOP";
      const char* download = "download";
      
      std::string fileName;
      std::cin >> fileName; //Getting file being requested to be downloaded
      
      sendToServer(sockfd, download); //Sending command 
      recvFromServer(sockfd, server_reply,1); // receiving which file user wishes to downloaded
      
      sendToServer(sockfd, fileName.c_str()); // sending file to be downloaded
      recvFromServer(sockfd, server_reply,1);// receiving if file exists or not
      
      std::string response = server_reply; 
      
      std::ifstream infile(fileName);       
      if(response == "READY" )
	{//file exists on server 
	  std::cout<< "File Exists on server!" << std::endl;
      	  
	  if(!infile.good())// While the file is not on the client
	    {
	      std::ofstream outfile(fileName);
	      
	      sendToServer(sockfd, ready); // Send ready message to begin download
	      
	      recvFromServer(sockfd, server_reply,0); // Receiving file
	      
	      
	      const char *success = "File received  Successfully";
	      sendToServer(sockfd, success);
	      // Store the file read from server into a string
	      // Write contents to file 
	      std::string file = server_reply;
	      outfile << file;
	      
	      std::cout << "File: \"" << fileName <<  "\" Downloaded!" << std::endl;
	      outfile.close(); // close output file		
	    }
	  
	  
	  //end  if(!infile.good())
	  else if(infile.good()==1) //check if file exists on local machine 
	    {
	      //prompt user to overwrite file or not
	      std::cout << "File Exists locally do you wish to overwrite file? (y/n)" << std::endl;
	      std::cin >> response ;
	      std::string lowResponse = modifyInput(response); // Make whatever user enters lowercase
	      
	      while(lowResponse != "y" && lowResponse != "n")
		{ // validate user input
		  std::cout << "Please enter (y/n) " << std::endl;
		  std::cin >> lowResponse ;
		}// end while
	      
	      if(lowResponse == "y")
		{ 
		  sendToServer(sockfd, ready);//send ready message to server
		  recvFromServer(sockfd, server_reply,0); //receive file		  
		  const char *success = "File received  Successfully";
		  sendToServer(sockfd, success); // send success message		  
		}// end if 
	      else 
		{ // Do not overwrite file
		  sendToServer(sockfd, stop);
		  recvFromServer(sockfd, server_reply,1);
		}//end else 
	      
	      infile.close(); // close input file
	      
	    } // end if(input == "READY")
	}
      // end else if(infile.good() == 1) 
    }
  //end if ready message 
  // end if download command selected 
  else if (input== "bye")
    { 
      const char* bye = "bye"  ; // bye message to be sent 
      sendToServer(sockfd, bye);
      recvFromServer(sockfd, server_reply,1); // receive server disconnected message
    } // end else if 
  else 
    {//invalid menu option 
      std::cout<< "Invalid option entered please enter a valid menu option" << std::endl ;
    }//end else
} // end valInput

/************************************************************************/
/* Function name: modifyInput                                        */
/* Description: Changes whatever user input was to be all lowercase   */
/* Parameters: string input-  The user input    */
/* Return Value: string - the new string that will be users input all  */
/*                        lowercase */
/*************************************************************************/  
std::string modifyInput(std::string input)
{
  
  std::string message;  
  for(int i =0; i < input.length(); i++)
    {//loop through string and make every character lowercase
      message.push_back(std::tolower(input[i]));
    } // end for 
  return message ;  
}
