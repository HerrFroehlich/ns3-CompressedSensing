%% SETTINGS
NOF_SRCNODES = 58;
NOF_FIELDS = 8;
NOF_LINES = 2313683; %only used for progressed bar
OUTDIR = './out/';
BUFSIZE = 30000; %buffer for non matching dates
DATE_INIT = '2004-02-28'; %date when recordings start
DATE_NCHARS = 10; %NOF chars in date string
DATE_DRANGE = DATE_NCHARS-1 : DATE_NCHARS; %position of day
DATE_MRANGE = DATE_NCHARS-4 : DATE_NCHARS-3; %position of month
%% read input file
fprintf('--------SPLITTING INTEL LAB DATA-------\n');
filename = input('Open file: ', 's');
h = waitbar(0,['READING INPUT FILE 0/' num2str(NOF_LINES)]);
fid = fopen(filename, 'r');

if(fid < 0)
    error('no such file!');
end

%% Create directories
for i = 1:NOF_SRCNODES
    success = mkdir([OUTDIR num2str(i)]);
    if(~success)
        error('cannot create directory');
    end
end
%% split lines to individual binary files
lineNumber=0;
buffer = cell(BUFSIZE, NOF_FIELDS);
nBuf = 0;
idOld=1;

fmarg = fopen([OUTDIR 'margins.txt'],'w');
fprintf(fmarg, 'Node1: 1-');

dir = [OUTDIR '1/'];

dateOld = DATE_INIT;
flog = fopen([dir 'split.log'],'a');
fdate = fopen([dir 'date'],'a');
ftime = fopen([dir 'time'],'a');
fepoch = fopen([dir 'epoch'],'a');
ftemp = fopen([dir 'temp'],'a');
fhum = fopen([dir 'hum'],'a');
flight = fopen([dir 'light'],'a');
fvolt = fopen([dir 'volt'],'a');


while(true)
    
    lineNumber = lineNumber+1;
    %fprintf('Scanning Line: %d \n',lineNumber);
    if(isvalid(h))
        msg = ['READING INPUT FILE ' num2str(lineNumber) '/' num2str(NOF_LINES)];
        waitbar(lineNumber/NOF_LINES, h, msg);
    else
        warning('ABORTING');
        break;
    end
    line = fgetl(fid);
    if(line < 0) break; end
    strings = strsplit(line);
    if((numel(strings)==NOF_FIELDS)||(numel(strings)==NOF_FIELDS-1))
        date = strings{1};
        time = strings{2};
        epoch = uint16( str2double(strings{3}));
        nodeIdNum = uint8( str2double(strings{4}));
        nodeId = strings{4};
        temp = str2double(strings{5});
        hum = str2double(strings{6});
        %some light measurements were apparently missing
        if(numel(strings)==NOF_FIELDS-1)   
            fprintf(flog,'Size mismatch! Assuming light sensor missing... for Node %d in line %d\n'...
                , nodeIdNum, lineNumber);
            light = 0.0;
            volt = str2double(strings{7});
        else
            light = str2double(strings{7});
            volt = str2double(strings{8});
        end
        %write buffer to files, open other files if new id
        if( nodeIdNum ~= idOld)
            
            dir = [OUTDIR nodeId '/'];
            dateOld = DATE_INIT;
            %since epoch may be overrun multiple times, we have to sort
            bufTemp = sortrows(buffer(1:nBuf, :),[8,3]);
            
            for i = 1:nBuf
                
                fprintf(fdate, '%s\n', buffer{i, 1});
                fprintf(ftime, '%s\n', buffer{i, 2});
                fwrite(fepoch, buffer{i, 3}, 'uint16');
                fwrite(ftemp, buffer{i, 4}, 'double');
                fwrite(fhum, buffer{i, 5}, 'double');
                fwrite(flight, buffer{i, 6}, 'double');
                fwrite(fvolt, buffer{i, 7}, 'double');
            end
            nBuf = 0;
            
            fprintf(fmarg, '%d\n', lineNumber-1);
            idOld = nodeIdNum;
            fprintf(fmarg, 'Node%d: %d-', idOld, lineNumber);
            
            fclose(flog);
            flog = fopen([dir 'split.log'],'a');
            
            fclose(fdate);
            fdate = fopen([dir 'date'],'a');
            
            fclose(ftime);
            ftime = fopen([dir 'time'],'a');
            
            fclose(fepoch);
            fepoch = fopen([dir 'epoch'],'a');
            
            fclose(ftemp);
            ftemp = fopen([dir 'temp'],'a');
            
            fclose(fhum);
            fhum = fopen([dir 'hum'],'a');
             
            fclose(flight);
            flight = fopen([dir 'light'],'a');

            fclose(fvolt);
            fvolt = fopen([dir 'volt'],'a');
        end
        
        %date jump?
        bSame = ~any(date - dateOld);%same date
        
        day = str2double(date(DATE_DRANGE));
        month = str2double(date(DATE_MRANGE));
        dayDiff = day - str2double(dateOld(DATE_DRANGE));
        bNewDay = (dayDiff == 1);%new day
        monthDiff = month - str2double(dateOld(DATE_MRANGE));
        bNewMonth = (monthDiff==1)...
                  && (day == 1);%new month
        
        if(bNewDay || bNewMonth)
            dateOld = date;
        end
        if(bNewDay || bNewMonth || bSame)%print to file
            fprintf(fdate, '%s\n', date);
            fprintf(ftime, '%s\n', time);
            fwrite(fepoch, epoch, 'uint16');
            fwrite(ftemp, temp, 'double');
            fwrite(fhum, hum, 'double');
            fwrite(flight,light, 'double');
            fwrite(fvolt,volt, 'double');
        else % save in buffer
            
            fprintf(flog,'Date mismatch! Rearranging line %d\n', lineNumber);
            nBuf = nBuf+1;
            buffer{nBuf, 1} = date;
            buffer{nBuf, 2} = time;
            buffer{nBuf, 3} = epoch;
            buffer{nBuf, 4} = temp;
            buffer{nBuf, 5} = hum;
            buffer{nBuf, 6} = light;
            buffer{nBuf, 7} = volt;
            buffer{nBuf, 8} = month*31+day; % use this for sorting
        end
        
    else
        warning('Size mismatch! Dropping line %d\n', lineNumber);
        fprintf(flog,'Size mismatch! Dropping line %d\n', lineNumber);        
    end
    
end
%% close files
fclose(fid);
fclose(fmarg);
fclose(flog);
fclose(fdate);
fclose(ftime);
fclose(fepoch);
fclose(ftemp);
fclose(fhum);
fclose(flight);
fclose(fvolt);
if(isvalid(h))
    close(h); 
end
fprintf('--------FINISHED-------\n');
