## Tech-Shell-Project  
LA Tech CSC 222 TechShell Project  

Name: Natalie Gates  

Description: This is a simple shell program, that is, 
a program that takes in user input and, if it is valid, 
runs the command provided  

To use this in a unix-like environment, simply clone the repo 
(git clone {url}), compile the c file (gcc techshell.c -o techshell), then
run the outputted executable (./techshell)  

Will break if the user uses a ~ in the directory they provide to cd and 
their HOME path environment is for some reason not set  

When the user doesn't put a closing ", simply acts as if there was one at
the end of their input  