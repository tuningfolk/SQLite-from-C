### Some questions to ponder upon
What format is data saved in? (in memory and on disk)  
When does it move from memory to disk?  
Why can there be only one primary key per table? (?)  
How does rolling back a transaction work (?)  
How are indexes formatted (?)  
When and how does a full table scan work (?)  
What format is a prepared statement saved in ?  

## Conceptual Framework

### Points to note
sqlite has fewer features thatn MySQL or PostgreSQL so easier to code.  
The entire database is stored in a single file.  

[C]: lseek returns the resulting offset position of the pointer as measured in bytes from the beginning of the file.  

Things you might want to do with cursors:  
* Create a cursor at the beginning of the table.
* Create a cursor at end of the table
* Access the row the cursor is pointing to.
* Advance the cursor to the next one

### SQLite architecture
![title](Images/design.gif)

The "front-end" of sqlite is a SQL(?) compiler that parses a string and outputs an internal representation called bytecode.  
This bytecode is passed to the virtual machine, which executes it.  
[SQLite Architecture](https://www.sqlite.org/arch.html)

### B-Tree
| |**B-tree**|**B+ tree**|
|-----|-----|-----|
Pronounced|“Bee Tree”|“Bee Plus Tree”
Used to store|Indexes|Tables
Internal nodes store keys|Yes|Yes
Internal nodes store values|Yes|No
Number of children per node|Less|More
Internal nodes vs. leaf nodes|Same structure|Different structure

m = tree's order
ceil(m/2)<=children of each node<=m



## Jargon

Serialization ([with the usage of void pointers](/del.c)): The process of converting an object’s state to a byte stream. This byte stream can then be saved to a file, sent over a network, or stored in a database  

[Virtual machine](https://www.techtarget.com/searchstorage/definition/virtual-memory): A memory management technique where [secondary memory](https://www.bing.com/search?q=secondary+memory+in+computer&cvid=ae75fbf27e164651b70bd570aac2b748&aqs=edge.1.0l9.9698j0j4&FORM=ANAB01&PC=DCTS) can be used as if it were a part of the [main memory](https://www.bing.com/search?pglt=41&q=main+memory+of+computer&cvid=8747c757f96d49748c19066db41561d2&aqs=edge.1.69i57j0l8.4195j0j1&FORM=ANAB01&PC=DCTS). Virtual memory is a common technique used in a computer's operating system (OS).  

[File Descriptor](https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/): An integer that uniquely identifies an open file of the process.  
File Descriptor table: Collection of integer array indices that are file descriptors in which elements are pointers to file table entries. One unique file descriptors table is provided in the operating system for each process.



