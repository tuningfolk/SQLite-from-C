### Some questions to ponder upon
What format is data saved in? (in memory and on disk)\n
When does it move from memory to disk?
Why can there be only one primary key per table? (?)
How does rolling back a transaction work (?)
How are indexed formatted (?)
When and how does a full table scan work (?)
What format is a prepared statement saved in ?

### Points to note
sqlite has fewer features thatn MySQL or PostgreSQL so easier to code
The entire database is stored in a single file


### SQLite architecture
![title](Images/design.gif)

The "front-end" of sqlite is a SQL(?) compiler that parses a string and outputs an internal representation called bytecode.
This bytecode is passed to the virtual machine, which executes it.
![SQLite Architecture](https://www.sqlite.org/arch.html)(Images/arch.gif)

### All the things you need to familiarize yourself with

Serialization: The process of converting an object’s state to a byte stream. This byte stream can then be saved to a file, sent over a network, or stored in a database

![void pointers](/projects2/repos/SQLite-from-C/del.c)void pointers:


