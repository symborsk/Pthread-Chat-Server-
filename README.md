  Assignment 2 - 379
   _____                .__                                     __    ________             _________________  ________ 
  /  _  \   ______ _____|__| ____   ____   _____   ____   _____/  |_  \_____  \            \_____  \______  \/   __   \
 /  /_\  \ /  ___//  ___/  |/ ___\ /    \ /     \_/ __ \ /    \   __\  /  ____/    ______    _(__  <   /    /\____    /
/    |    \\___ \ \___ \|  / /_/  >   |  \  Y Y  \  ___/|   |  \  |   /       \   /_____/   /       \ /    /    /    / 
\____|__  /____  >____  >__\___  /|___|  /__|_|  /\___  >___|  /__|   \_______ \           /______  //____/    /____/  
        \/     \/     \/  /_____/      \/      \/     \/     \/               \/                  \/                 

        									By John Symborski & Aaron Philips
        									     1387814          1408127

-------------------------------------------------------------------------------------------------------------------------

Client interface:
-----------------------------

Entering the chat room: When a user enters the chat room it immediatley prints out all the usernames that are currently
in the chat already in the format : "<username>  has been added to chat room". After these messages are printed and the client will
see "<their username> has been added to chat room". After that point the user can chat in chat room.

People entering\leaving: When you are in the chat room and someone enters/leaves it simply prints out "<username> has been
added/removed from chat room."

Printing out all current usernames: When you want to print out and view all the current usernames the client must type ".usernames".
This will then print out each username. This does not send a message to the server but simply prompt client server to print out the
list that it has.

Exiting the chat room: In order to ensure the client shut downs correctly the user must hit CTLR C in order to exit and terminate 
client.

Important note: Please note that for our client server interface to break up confusion over user messages and user update messages there is always a space made right before a update message. So an update message would look like:
 John has been added to chat room -- user update message
Aaron : Hey did you know A & W has 2 teen burgers for 8 bucks? -- user message
---------------------------------------------------------------------------------------------------------------------------