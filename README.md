# LBackup program

This is a LBackup program (LightBackup), it 
launches a daemon which monitors specified directory
for any modifications and, should any occur or in 
case no modifications at all have ever been introduced to
the directory, makes a copy of a directory to another backup directory


IMPORTANT NOTE: the program creates log file in /var/log, therefore it ought
to be launched under sudo.

Usage: `./lbp <src> <dest> [links_type]`

<src> - the directory to be copied, <dest> - the backup directory, <links_type> - information about links. Use `-H` not to be dereferenced and `-L` for dereferencing
`-H` behavour is set by default if no other option is present.

Please send all ideas and bug reports to sidelnikov.si.001@gmail.com

