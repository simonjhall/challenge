function convertbinary(outname,inname)
% Accepts a binary image in inname and writes out an assembler file
% of image data
% Based on Arial 24 point with 50 kerning
global pos I2

if (nargin<2)
    error('Specify an output filename followed by an input filename e.g. convertfont fontdata.s font.bmp')
end

A=double(imread(inname));
fid=fopen(outname,'w');
if (fid<=0) 
    return
end
fprintf(fid,'; Generated from %s\n',inname);
fprintf(fid,'.globl fontdata\n');
fprintf(fid,'.globl fontpos\n');
fprintf(fid,' .align 32\n');
fprintf(fid,'fontpos:\n');

% Each character is stored in a length 16 vector of words
[a,b,c]=size(A);
if (a~=32) 
    error('Image must be 32 down for font conversion')
end

% Now we search through for lines of all white pixels
I=sum(sum(A,3));
I2=(I==3*255*32);

% We want to start off with pos pointing at the first black of a character
% and with letterstart at the pos of the start of the letter
pos=1;
findblack

letterend=[];

for letter=1:(128-32)
    % Scan for the beginning of the white
    findwhite
    posa=pos;
    findblack
    letterend(letter)=floor((posa+pos)/2);  
    
    fprintf(fid,'\t.half\t0x%x\n',letterend(letter));
    
    % Lets display each of these
    if (letter>1)
        subplot(10,10,letter)
        image(uint8(A(:,letterend(letter-1):letterend(letter),:)))
        axis image
        if (letter<128-32)
           title(sprintf('%c',letter+32-1))
        end
        drawnow
    end
end

fprintf(fid,'fontdata:\n');


for y=1:b
        asum=0;
        for x=32:-1:1
            asum=asum+(A(x,y,1)>50)*(2^(32-x));
        end
        fprintf(fid,'\t.word\t0x%x\n',asum);
end


fclose(fid);

function findblack
global pos I2
% Scan through until we find a black position
% stop with pos==black line
while(I2(pos)==1)
    pos=pos+1;
end

function findwhite
global pos I2
% Scan through until we find a black position
% stop with pos==black line
while(I2(pos)==0)
    pos=pos+1;
end