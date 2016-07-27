//Made by Grant Espe and John Bieber

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int globalTime;

//Cache Structs

typedef struct _line{
    int tag;
    int age;
    int dBit; 
} Line;

typedef struct _set{
    Line* lines;   
} Set;

typedef struct _cache{
    Set* sets;
    int numSets;
    int numLines;
    int numBytes;
} Cache;

//Non Cache Structs

typedef struct _summary{
    int hits;
    int misses;
    int evicts;
} Summary;


typedef struct _op{
    char type;
    unsigned int address;
    int size;
} Operation;

//For containing the Length in the return value
typedef struct _opSet{
    Operation* ops;
    int length; // the current size of the set
} OpSet; 
    
OpSet* parseTrace(FILE *trace);
Summary* simulate(OpSet* opList, int setsExp, int linesPerSet, 
                                    int bytesExp, int isVerbose);

char loadStore(Cache* theCache, unsigned int target);
char modify(Cache* theCache, unsigned int target);

void printCache(Cache* theCache);
void printHelp();
void printMissingArgsHelp();

void printSummary2(int hits, int misses, int evictions);

/** Parse the Input Arguments,
* Parse the Trace file
* Then Simulate...
*/
int main(int argc, char* argv[]){

    char* traceName;
    int hasTrace = 0;

    int setsExp;
    int hasS = 0;

    int linesPerSet;
    int hasE = 0;

    int bytesExp;
    int hasB = 0;

	int verbose = 0;

	//Parse Arguments
	for(int k = 1; k < argc; k++){
		char* anArg = argv[k];
		char firstChar = anArg[0];

		if(firstChar == '-'){
            int notLastArg = (k+1 < argc);
            char nextChar = anArg[1];

            switch(nextChar){

			case 'h': 
				printHelp();
			    break;

			case 'v':
				verbose = 1;
			    break;

            case 't':
                if(notLastArg){
                    traceName = argv[k+1];
                    hasTrace = 1;
                    k+=1;
                }
                else
                    printMissingArgsHelp();
                break;

            case 's':
                if(notLastArg && sscanf(argv[k+1], "%d", &setsExp) && setsExp > 0){
                    //next arg into setsExp, and check that it was an int
                    hasS = 1;
                    k+=1;                  
                }
                else
                    printMissingArgsHelp();

                break;
            case 'E':
                if(notLastArg && sscanf(argv[k+1], "%d", &linesPerSet) && linesPerSet > 0){
                        hasE = 1;
                        k+=1; 
                }
                else
                    printMissingArgsHelp();

                break;
            case 'b':
                if(notLastArg && sscanf(argv[k+1], "%d", &bytesExp) && bytesExp >0){
                        hasB = 1;
                        k+=1;
                }
                else
                    printMissingArgsHelp();    
                
                break;
            }
		}

	} // End Parsing arguments

    //If its missing arguments
    if(!(hasB && hasE && hasS && hasTrace)){
        printMissingArgsHelp();
    }
    else{
        //Open the trace to read
        FILE* trace = fopen(traceName, "r");
        OpSet* ops = parseTrace(trace);
	
	/*int i = 0;
	for(i = 0; i < ops->length; i++){
	printf("%c %x %d", ops->ops[i].type, ops->ops[i].address, ops->ops[i].size);
	printf("\n");
	}*/

      //printf("\nTraces Parsed\n");

        Summary* results = simulate(ops, setsExp, linesPerSet, bytesExp, verbose);
        printSummary2(results->hits, results->misses, results->evicts);

        fclose(trace);
    }

    
    return 0;
}

//Converts a file buffer into a set of Operation structs
OpSet* parseTrace(FILE *trace){

  int lineSize = 63; // the maximum number of characters to read per line
  char nextLine[lineSize];
  int sizecount = 0;

  // fgets returns '\0' when it hits EOF and nothing has been read
  while(fgets(nextLine, lineSize, trace)){ // while
    char opType; // What type of operation should be performed (I, L, S, M expected)
    unsigned int hexVal; // The hexadecimal value given for the operation address
    int bytes; // The number of bytes accessed by the operation

    int converted;
    converted = sscanf(nextLine, " %c %x,%d", &opType, &hexVal, &bytes);
    if(converted == 3 && opType != 'I'){ // if
      sizecount++;
    } else {} // Do nothing, close if
  }// close while

    rewind(trace);// go back to the start of the file

//-----------------------------------------------------------------------------------//
    
    Operation* opList = (Operation* ) calloc(sizecount, sizeof(Operation)); // create an array of Operations for the OpSet
    OpSet* opset;
    opset = (OpSet* ) malloc(sizeof(OpSet));
// OpSet has ops and length
// ops = Operation*
// length = int

    opset->ops = opList;
    opset->length = sizecount;

    int index = 0;
    // Now add the operations to the array
    while(fgets(nextLine, lineSize, trace)){
      char opType; // What type of operation should be performed (I, L, S, M expected)
      unsigned int hexVal; // The hexadecimal value given for the operation address
      int bytes; // The number of bytes accessed by the operation

      int converted = 0;
      converted = sscanf(nextLine, " %c %x,%d", &opType, &hexVal, &bytes);
      if(converted == 3 && opType != 'I'){
        // add the operations to the list
        opset->ops[index].type = opType;
        opset->ops[index].address = hexVal;
        opset->ops[index].size = bytes;
        index++;
      } else {} // Do nothing
    }
  return opset;
}

//Initialize a cache struct, with filled fields.
Cache* initCache(int setsExp, int linesPerSet, int bytesExp){
    int bytesPerLine = 1;
    int sets = 1;   

    //finds 2^n without using the math.h pow function
    bytesPerLine <<= bytesExp;

    sets <<= setsExp;


    //Initialize the Cache
    Cache* theCache = (Cache*)malloc(sizeof(Cache));
    
    Set* theSets = (Set*)calloc(sets, sizeof(Set));

    theCache->sets = theSets;
    theCache->numBytes = bytesPerLine;
    theCache->numLines = linesPerSet;
    theCache->numSets = sets;

    for(int k = 0; k < sets; k++){
        //Calloc an array of Lines for each set
        theCache->sets[k].lines = (Line*)calloc(linesPerSet, sizeof(Line));


        //Set Ages of all lines to 0
        //Set all Lines to -1 (to indicate it is empty)
        for(int i = 0; i < linesPerSet; i++){
            //0 will indicate empty line
            theCache->sets[k].lines[i].tag = -1;
            theCache->sets[k].lines[i].age = 0;
            theCache->sets[k].lines[i].dBit = 0;
        }

        /*for(int i = 0; i < linesPerSet; i++){
            //Calloc an array of ints (bytes) for each set
            theCache->sets[k].lines[i].bytes = (int*)calloc(bytesPerLine, sizeof(int));
        */
    }

    return theCache;
}

//Prints out the sets, lines, tags and ages of a cache object
//useful for debugging
void printCache(Cache* theCache){

    for(int k = 0; k < theCache->numSets; k++){
        //Calloc an array of Lines for each set

        printf("SET - %d\n", k);

        //Set Ages of all lines to 0
        //Set all Lines to -1 (to indicate it is empty)
        for(int i = 0; i < theCache->numLines; i++){
            //0 will indicate empty line
            printf("  Line = %d\n", i);

            printf("     tag = %d\n",theCache->sets[k].lines[i].tag);
            printf("     age = %d\n", theCache->sets[k].lines[i].age);
        }

    }

}

//Simulate the caching operations
//Returns a summary of hits, misses, and evictions
Summary* simulate(OpSet* opList, int setsExp, int linesPerSet, 
                                    int bytesExp, int isVerbose){

    globalTime = 0;

    int misses = 0;
    int hits = 0;
    int evicts = 0;

    int numOps = opList->length;
    Operation* operations = opList->ops;

    Cache* theCache = initCache(setsExp,linesPerSet,bytesExp);

    //printCache(theCache);

    //printf("\nCache Initialized\n");

    //Start Cycling through the operations
    for(int k = 0; k < numOps; k++){

        globalTime += 1;

        //Deconstruct the operation
        Operation currentOp = operations[k];
        char type = currentOp.type;
        unsigned int target = currentOp.address;
        int size = currentOp.size;

        switch(type){
            //Load and store are the same
            case 'L':
            case 'S':
                    switch(loadStore(theCache, target)){
                            case 'M':
                                misses++;
                                if(isVerbose)
                                    printf("%c %x,%d miss\n", type, target, size);
                                break;
                            case 'H':
                                hits++;
                                if(isVerbose)
                                    printf("%c %x,%d hit\n", type, target, size);
                                break;
                            case 'E':
                                misses++;
                                evicts++;
                                if(isVerbose)
                                    printf("%c %x,%d miss evict\n", type, target, size);
                                break;
                    }
                break;          
            case 'M':
                switch(loadStore(theCache, target)){
                            case 'M':
                                misses++;
                                hits++; //Because the store will always hit
                                if(isVerbose)
                                    printf("%c %x,%d miss hit\n", type, target, size);
                                break;
                            case 'H':
                                hits++;
                                hits++;//
                                if(isVerbose)
                                    printf("%c %x,%d hit hit\n", type, target, size);
                                break;
                            case 'E':
                                misses++;
                                evicts++;
                                hits++;// The store always hits
                                if(isVerbose)
                                    printf("%c %x,%d miss evict hit\n", type, target, size);
                                break;
                            }
                break;
                }


    }

    Summary* results = malloc(sizeof(Summary));
    results->misses = misses;
    results->hits = hits;
    results->evicts = evicts;

    free(theCache);
    return results;


}


//Simulates loading or storing a number of addresses to/from a cache
//returns a character indication Miss, Hit or Evict
char loadStore(Cache* theCache, unsigned int target){
    char result = 'M';

    //Calculate the modulo value for wrapping the cache sets (bytes per column)
    int bytesPerCol = theCache->numSets * theCache->numBytes;


    int whichSet = (int) ((double)(target % bytesPerCol) / (double)theCache->numBytes);

    //Rounds down so it only will check the bottom byte of the line
    //  Rounds down to bottom of line
    unsigned int roundDownTarget = target - (target % theCache->numBytes);

    //printf("\nbytesPerCol = %d, whichSet = %d, target = %u, roundDownTarget = %u\n",
    //    bytesPerCol, whichSet, target, roundDownTarget);

    //First checks for a hit.
    for(int k = 0; k < theCache->numLines; k++){
        if(theCache->sets[whichSet].lines[k].tag == roundDownTarget){

            //Set current age to globalTime (so it will always be highest)
            theCache->sets[whichSet].lines[k].age = globalTime;

            return 'H';
        }
    } // No hit...

    //Find which line to evict

    int min = globalTime;
    int minIndex = -1;
      
    //Cycle through the ages, find lowest
    for(int k = 0; k < theCache->numLines; k++){
        int thisAge = theCache->sets[whichSet].lines[k].age;
            if(thisAge < min){

                //printf("min = %d, minIndex = %d, thisAge = %d\n", min, minIndex, thisAge);
                min = thisAge;
                minIndex = k;
            }
    }

    //printf("\nfound lowest age: %d\n", min);

    //Check if the line was empty (dBit = 0), thus no evict
    if(theCache->sets[whichSet].lines[minIndex].dBit == 0){
        theCache->sets[whichSet].lines[minIndex].dBit = 1;
        result = 'M';
    }
    else
        result = 'E';

    //Store the new value into the proper place in the cache
    theCache->sets[whichSet].lines[minIndex].tag = roundDownTarget;
    //Update its age
    theCache->sets[whichSet].lines[minIndex].age = globalTime;


    return result;

}


//


/*prints the missing args line and the help lines
*/
void printMissingArgsHelp(){
    printf("./csim-ref: Missing required command line argument\n");
    printHelp();
}

/** Prints out the Help message
*@return void
*/

void printHelp(){
    printf("./csim: Missing required command line argument\n");
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");

    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

//Prints hits misses and evicts
void printSummary2(int hits, int misses, int evictions){
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
}









