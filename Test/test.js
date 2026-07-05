//#region toolkit.js

/**
 * this function prints all the input arguments with no space in between and with a new line at the end
 * @param  {...any} input inputs of the print aguments are printed one by one with no space in between
 */
function println(...input) {

    //create a string variable to store the input as one big string to be printed
    let output = "";

    //iterate through all of the arguments
    for(let i = 0; i < input.length; i++) {
        //get the current index
        let curr = input[i];

        if(typeof(curr) == "object") {
            //Lets use a custom function that I don't fucking understand and that I wish I did
            output += JSON.stringify(curr, null, 2);
        }else {
            //add the current index to the output (convert to a string in the process)
            output += curr;
        }
    }
    
    //print current line
    console.log(output);
    
    //clear the output
    output = "";
}


/**
 * 
 * @param {} ms the desired amount of milliseconds you want to stop the async function for (gives execution to other parts of the code)
 * @returns return a promise that gives back async function execution after the desired amount of milliseconds `ms`
 */
function delay(ms) {
    //return a promise that gives back async function execution after the desired amount of milliseconds "ms"
    return new Promise(
        //set a lambda that calls resolve after the desired amount of milliseconds "ms"
        function(resolve) {
            //timeout to call resolvea fter a desired amount of milliseconds "ms"
            setTimeout(resolve, ms);
        }
    );
}

function getLineNumber() {
    const stack = (new Error()).stack;
    // Look at the text, find the second file reference, and pull the line number
    const match = stack.split('\n')[2].match(/:(\d+):\d+\)?$/);
    return match ? Number(match[1]) : null;
}
//#endregion