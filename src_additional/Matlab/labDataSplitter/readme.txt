- Before restarting: delete out directory, else data will be appended to existing files!
- for each node data is splitted into an individual directory
- The following output files will be create:
	- date : text file, containing dates in each line with the following form: yyyy-mm-dd
	- epoch : binary file with uint16 values, containing the eopch numbers
	- hum : binary file with double values, containing the humidity readings
	- light : binary file with double values, containing the light readings
	- split.log : text file, log for the split process
	- temp : binary file with double values, containing the temperarture readings
	- light : binary file with double values, containing the light readings
	- time : text file, containing times in each line with the following form: hh-mm-ss.xxxx
	- volt : binary file with double values, containing the voltage readings

FIXED ISSUES:
-for some sensors sometimes light measurements are missing => set light values to zero in that case
-data is sorted by 1)moteID 2)epoch, but epoch overuns (apparently twice) -> date jumps => sorted while parsing
-in the file there are 58 moteIDs, where as the webpage clames that there were only 54 

SENSOR ISSUES (NOT complete):
all	 : some epochs missing
sensor 5 : no valid data (except some voltage readings)
sensor 7 : some corrupted lines
sensor 8 : beginning from line 276858, some light measurements are missing
sensor 9 : light measurements missing
sensor 12 : same, light sensor readings missing for a long time