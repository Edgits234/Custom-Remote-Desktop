#ifndef CMD_HANDLE_H 
#define CMD_HANDLE_H

#include <toolkit.h>

#define CMD_IDS(...) \
enum CMD_ID : uint8_t { __VA_ARGS__ }; \
const char* str_ids = #__VA_ARGS__;

namespace CMD {

    //declare commands
    CMD_IDS(
        LOGIN ,
        SIGNIN,
    );

    //login handle
    void login(const uint8_t* data, int len) {
        println("handling login with data : ");
        printlnlen((const char*)(data), len);
    }

    //handle for struct printing when using iostream
    std::ostream& operator<<(std::ostream& os, const CMD_ID id) {
        //counter to remember how many commas we found
        int find_count = -1;

        //find all occurences of commas
        int i    = -1;
        int last = -1;
        while(true) {
            //if repeated means last iteration we found a comma
            find_count++;

            //update index storer variables
            i++;
            i = findstr(str_ids, ",", i); //store current comma

            //check for getting out loop condition
            if(!(i >= 0 && find_count != (int)(id))) {
                break;
            }

            last = i;                     //store last comma 
        }
        
        //if find ended before we could find id (means that id is not in the list)
        if(find_count < (int)(id)) {
            //print inexistent id to signify this
            os << "InexistentID";
        }else {
            //get start and end of correct CMD ID (includes whitespaces)
            int start = last + 1;
            int end   = i - 1   ;

            //skip whitespace characters
            while(str_ids[start] == ' ' || str_ids[start] == '\n' || str_ids[start] == '\t') start++;
            while(str_ids[end  ] == ' ' || str_ids[end  ] == '\n' || str_ids[end  ] == '\t') end  --;

            //print the CMD ID
            const char* str = str_ids + start;
            int         len = end - start + 1;
            for(int i = 0; i < len; i++)
            {
                os << str[i];
            }
        }

        return os;
    }

    //define a cmd bind (associate a command ID with a function)
    struct CmdBind {
        CMD_ID id                    ; // id of the command
        void (*fn)(const uint8_t* data, int len); // the function to excecute when seeing this command
    };

    //array to define those command binds
    CmdBind cmd_binds[] = {
        {CMD_ID::LOGIN, login},
    };

    //handle incomming data from the internet stream going to the credential server
    void handle(const uint8_t* data, int len) {
        CMD_ID cmd_id = (CMD_ID)(data[0]); //get the first character (the CMD id)
        data++; len--;                     //skip first byte (its the cmd_id)

        //check if we played a function
        bool success = false;

        //go through array of cmd_binds and play first fn that matches id we recieved
        for(int i = 0; i < sizeofarray(cmd_binds); i++) {
            //get current binding
            CmdBind& curr = cmd_binds[i];

            //if matches the id play it
            if(curr.id == cmd_id) {
                //play function send data to it
                curr.fn(data, len);

                //indicate we did find a function corresponding to id
                success = true;
            }
        }

        if(success == false) {
            println("WARNING, did not find a function for \"",cmd_id,"\" command");
        }
    }

    //automatically handle conversions by forwarding to main function
    void handle(const char* str) {handle((const uint8_t*)(str)        , strlen_tk(str));}
    void handle(std::string str) {handle((const uint8_t*)(str.c_str()), str.length()  );}
};

#endif