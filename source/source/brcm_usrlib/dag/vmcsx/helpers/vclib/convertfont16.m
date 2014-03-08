function convertbinary(outname,inname)
% Accepts a binary image in inname and writes out an assembler file
% of image data
global pos I2

A=double(imread(inname));
fid=fopen(outname,'w');
if (fid<=0) 
    return
end
fprintf(fid,'; Generated from %s\n',inname);
fprintf(fid,'.globl fontdata16\n');
fprintf(fid,'.globl fontpos16\n');
fprintf(fid,' .align 32\n');
fprintf(fid,'fontpos16:\n');

% Each character is stored in a length 16 vector of words
[a,b,c]=size(A);
if (a~=16) 
    error('Image must be 16 down for font conversion')
end

% Now we search through for lines of all white pixels
I=sum(sum(A,3));
I2=(I==3*255*16);

% Now we search through for horontal lines of all white pixels
Z=sum(sum(255-A,3),2);

% Replace wide gaps between letters with single pixel gap
I3=I2(1:end-1) & I2(2:end);
A(:,I3,:)=[];

% Move black pixels to top
A(Z==0,:,:)=[];
A(size(A,1)+1:16, :,:)=255;

[a,b,c]=size(A);
if (a~=16) 
    error('Image must be 16 down for font conversion')
end

% Now we search through for lines of all white pixels
I=sum(sum(A,3));
I2=(I==3*255*16);


% We want to start off with pos pointing at the first black of a character
% and with letterstart at the pos of the start of the letter
pos=1;
findblack

letterend=[];

for letter=1:(128-32)
    % Scan for the beginning of the white
    findwhite
    posa=pos;
    letterend(letter)=floor((posa+pos)/2);  
    findblack
    
    fprintf(fid,'\t.half\t0x%x\n',letterend(letter));
    
    % Lets display each of these
    if (letter>1)
        subplot(10,10,letter)
        image(uint8(A(:,letterend(letter-1):letterend(letter),:)))
        axis image
        drawnow
    end
end

fprintf(fid,'fontdata16:\n');


for y=1:b
        sum=0;
        for x=16:-1:1
            sum=sum+(A(x,y,1)>50)*(2^(16-x));
        end
        fprintf(fid,'\t.half\t0x%x\n',sum);
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