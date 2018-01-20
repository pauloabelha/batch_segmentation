%!PS-Adobe-3.0
%%Creator: (ImageMagick)
%%Title: (re)
%%CreationDate: (2017-10-17T19:22:14+01:00)
%%BoundingBox: 900 279 915 322
%%HiResBoundingBox: 900 279 915 322
%%DocumentData: Clean7Bit
%%LanguageLevel: 1
%%Orientation: Portrait
%%PageOrder: Ascend
%%Pages: 1
%%EndComments

%%BeginDefaults
%%EndDefaults

%%BeginProlog
%
% Display a color image.  The image is displayed in color on
% Postscript viewers or printers that support color, otherwise
% it is displayed as grayscale.
%
/DirectClassPacket
{
  %
  % Get a DirectClass packet.
  %
  % Parameters:
  %   red.
  %   green.
  %   blue.
  %   length: number of pixels minus one of this color (optional).
  %
  currentfile color_packet readhexstring pop pop
  compression 0 eq
  {
    /number_pixels 3 def
  }
  {
    currentfile byte readhexstring pop 0 get
    /number_pixels exch 1 add 3 mul def
  } ifelse
  0 3 number_pixels 1 sub
  {
    pixels exch color_packet putinterval
  } for
  pixels 0 number_pixels getinterval
} bind def

/DirectClassImage
{
  %
  % Display a DirectClass image.
  %
  systemdict /colorimage known
  {
    columns rows 8
    [
      columns 0 0
      rows neg 0 rows
    ]
    { DirectClassPacket } false 3 colorimage
  }
  {
    %
    % No colorimage operator;  convert to grayscale.
    %
    columns rows 8
    [
      columns 0 0
      rows neg 0 rows
    ]
    { GrayDirectClassPacket } image
  } ifelse
} bind def

/GrayDirectClassPacket
{
  %
  % Get a DirectClass packet;  convert to grayscale.
  %
  % Parameters:
  %   red
  %   green
  %   blue
  %   length: number of pixels minus one of this color (optional).
  %
  currentfile color_packet readhexstring pop pop
  color_packet 0 get 0.299 mul
  color_packet 1 get 0.587 mul add
  color_packet 2 get 0.114 mul add
  cvi
  /gray_packet exch def
  compression 0 eq
  {
    /number_pixels 1 def
  }
  {
    currentfile byte readhexstring pop 0 get
    /number_pixels exch 1 add def
  } ifelse
  0 1 number_pixels 1 sub
  {
    pixels exch gray_packet put
  } for
  pixels 0 number_pixels getinterval
} bind def

/GrayPseudoClassPacket
{
  %
  % Get a PseudoClass packet;  convert to grayscale.
  %
  % Parameters:
  %   index: index into the colormap.
  %   length: number of pixels minus one of this color (optional).
  %
  currentfile byte readhexstring pop 0 get
  /offset exch 3 mul def
  /color_packet colormap offset 3 getinterval def
  color_packet 0 get 0.299 mul
  color_packet 1 get 0.587 mul add
  color_packet 2 get 0.114 mul add
  cvi
  /gray_packet exch def
  compression 0 eq
  {
    /number_pixels 1 def
  }
  {
    currentfile byte readhexstring pop 0 get
    /number_pixels exch 1 add def
  } ifelse
  0 1 number_pixels 1 sub
  {
    pixels exch gray_packet put
  } for
  pixels 0 number_pixels getinterval
} bind def

/PseudoClassPacket
{
  %
  % Get a PseudoClass packet.
  %
  % Parameters:
  %   index: index into the colormap.
  %   length: number of pixels minus one of this color (optional).
  %
  currentfile byte readhexstring pop 0 get
  /offset exch 3 mul def
  /color_packet colormap offset 3 getinterval def
  compression 0 eq
  {
    /number_pixels 3 def
  }
  {
    currentfile byte readhexstring pop 0 get
    /number_pixels exch 1 add 3 mul def
  } ifelse
  0 3 number_pixels 1 sub
  {
    pixels exch color_packet putinterval
  } for
  pixels 0 number_pixels getinterval
} bind def

/PseudoClassImage
{
  %
  % Display a PseudoClass image.
  %
  % Parameters:
  %   class: 0-PseudoClass or 1-Grayscale.
  %
  currentfile buffer readline pop
  token pop /class exch def pop
  class 0 gt
  {
    currentfile buffer readline pop
    token pop /depth exch def pop
    /grays columns 8 add depth sub depth mul 8 idiv string def
    columns rows depth
    [
      columns 0 0
      rows neg 0 rows
    ]
    { currentfile grays readhexstring pop } image
  }
  {
    %
    % Parameters:
    %   colors: number of colors in the colormap.
    %   colormap: red, green, blue color packets.
    %
    currentfile buffer readline pop
    token pop /colors exch def pop
    /colors colors 3 mul def
    /colormap colors string def
    currentfile colormap readhexstring pop pop
    systemdict /colorimage known
    {
      columns rows 8
      [
        columns 0 0
        rows neg 0 rows
      ]
      { PseudoClassPacket } false 3 colorimage
    }
    {
      %
      % No colorimage operator;  convert to grayscale.
      %
      columns rows 8
      [
        columns 0 0
        rows neg 0 rows
      ]
      { GrayPseudoClassPacket } image
    } ifelse
  } ifelse
} bind def

/DisplayImage
{
  %
  % Display a DirectClass or PseudoClass image.
  %
  % Parameters:
  %   x & y translation.
  %   x & y scale.
  %   label pointsize.
  %   image label.
  %   image columns & rows.
  %   class: 0-DirectClass or 1-PseudoClass.
  %   compression: 0-none or 1-RunlengthEncoded.
  %   hex color packets.
  %
  gsave
  /buffer 512 string def
  /byte 1 string def
  /color_packet 3 string def
  /pixels 768 string def

  currentfile buffer readline pop
  token pop /x exch def
  token pop /y exch def pop
  x y translate
  currentfile buffer readline pop
  token pop /x exch def
  token pop /y exch def pop
  currentfile buffer readline pop
  token pop /pointsize exch def pop
  /Times-Roman findfont pointsize scalefont setfont
  x y scale
  currentfile buffer readline pop
  token pop /columns exch def
  token pop /rows exch def pop
  currentfile buffer readline pop
  token pop /class exch def pop
  currentfile buffer readline pop
  token pop /compression exch def pop
  class 0 gt { PseudoClassImage } { DirectClassImage } ifelse
  grestore
  showpage
} bind def
%%EndProlog
%%Page:  1 1
%%PageBoundingBox: 900 279 915 322
DisplayImage
900 279
15 43
12
15 43
0
0
F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6F5F5D6
F5F5D6F5F5D6F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5
F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5
F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F4F4D5F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4
F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F3F3D4F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3
F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F2F2D3F1F1D3F1F1D3F1F1D3
F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F1F1D3F0F0D2
F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2F0F0D2
F0F0D2EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1EFEFD1
EFEFD1EFEFD1EFEFD111112F11112F11112F11112F11112F11112F11112F11112F11112F11112F
11112F11112F11112FEEEED0EEEED0121230EDEDCFEDEDCFEDEDCFEDEDCFEDEDCFEDEDCFEDEDCF
EDEDCFEDEDCFEDEDCFEDEDCF121230EDEDCFEDEDCF131331ECECCEECECCEECECCEECECCEECECCE
ECECCEECECCEECECCEECECCEECECCEECECCE131331ECECCEECECCE141432EBEBCDEBEBCDEBEBCD
EBEBCDEBEBCDEBEBCDEBEBCDEBEBCDEBEBCDEBEBCDEBEBCD141432EBEBCDEBEBCD151533EAEACC
EAEACCEAEACCEAEACCEAEACCEAEACCEAEACCEAEACCEAEACCEAEACCEAEACC151533EAEACCEAEACC
171734E8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CBE8E8CB171734
E8E8CBE8E8CB181835E7E7CAE7E7CAE7E7CAE7E7CAE7E7CAE7E7CAE7E7CAE7E7CAE7E7CAE7E7CA
E7E7CA181835E7E7CAE7E7CA1A1A37E5E5C8E5E5C8E5E5C8E5E5C8E5E5C8E5E5C8E5E5C8E5E5C8
E5E5C8E5E5C8E5E5C81A1A37E5E5C8E5E5C81C1C39E3E3C6E3E3C6E3E3C6E3E3C6E3E3C6E3E3C6
E3E3C6E3E3C6E3E3C6E3E3C6E3E3C61C1C39E3E3C6E3E3C61E1E3AE1E1C5E1E1C5E1E1C5E1E1C5
E1E1C5E1E1C5E1E1C5E1E1C5E1E1C5E1E1C5E1E1C51E1E3AE1E1C5E1E1C520203CDFDFC3DFDFC3
DFDFC3DFDFC3DFDFC3DFDFC3DFDFC3DFDFC3DFDFC3DFDFC3DFDFC320203CDFDFC3DFDFC322223E
DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC1DDDDC122223EDDDDC1
DDDDC1252540DADABFDADABFDADABFDADABFDADABFDADABFDADABFDADABFDADABFDADABFDADABF
252540DADABFDADABF272742D8D8BDD8D8BDD8D8BDD8D8BDD8D8BDD8D8BDD8D8BDD8D8BDD8D8BD
D8D8BDD8D8BD272742D8D8BDD8D8BD2A2A45D5D5BAD5D5BAD5D5BAD5D5BAD5D5BAD5D5BAD5D5BA
D5D5BAD5D5BAD5D5BAD5D5BA2A2A45D5D5BAD5D5BA2C2C47D3D3B8D3D3B8D3D3B8D3D3B8D3D3B8
D3D3B8D3D3B8D3D3B8D3D3B8D3D3B8D3D3B82C2C47D3D3B8D3D3B830304BCFCFB4CFCFB4CFCFB4
CFCFB4CFCFB4CFCFB4CFCFB4CFCFB4CFCFB4CFCFB4CFCFB430304BCFCFB4CFCFB433334DCCCCB2
CCCCB2CCCCB2CCCCB2CCCCB2CCCCB2CCCCB2CCCCB2CCCCB2CCCCB2CCCCB233334DCCCCB2CCCCB2
373751C8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AEC8C8AE373751
C8C8AEC8C8AE3B3B54C4C4ABC4C4ABC4C4ABC4C4ABC4C4ABC4C4ABC4C4ABC4C4ABC4C4ABC4C4AB
C4C4AB3B3B54C4C4ABC4C4AB404058BFBFA7BFBFA7BFBFA7BFBFA7BFBFA7BFBFA7BFBFA7BFBFA7
BFBFA7BFBFA7BFBFA7404058BFBFA7BFBFA7A7A8B158574E58574E58574E58574E58574E58574E
58574E58574E58574E58574E58574EA7A8B158574E58574E9D9FAA626055626055626055626055
6260556260556260556260556260556260556260559D9FAA626055626055A5A8B05A574F5A574F
5A574F5A574F5A574F5A574F5A574F5A574F5A574F5A574F5A574FA5A8B05A574F5A574FA6A8B1
59574E59574E59574E59574E59574E59574E59574E59574E59574E59574E59574EA6A8B159574E
59574EA8AAB257554D57554D57554D57554D57554D57554D57554D57554D57554D57554D57554D
A8AAB257554D57554DA9ABB356544C56544C56544C56544C56544C56544C56544C56544C56544C
56544C56544CA9ABB356544C56544CAAACB455534B55534B55534B55534B55534B55534B55534B
55534B55534B55534B55534BAAACB455534B55534BABADB554524A54524A54524A54524A54524A
54524A54524A54524A54524A54524A54524AABADB554524A54524AACAEB553514A53514A53514A
53514A53514A53514A53514A53514A53514A53514A53514AACAEB553514A53514AADAFB6525049
525049525049525049525049525049525049525049525049525049525049ADAFB6525049525049
AEB0B7514F48514F48514F48514F48514F48514F48514F48514F48514F48514F48514F48AEB0B7
514F48514F48B0B1B84F4E474F4E474F4E474F4E474F4E474F4E474F4E474F4E474F4E474F4E47
4F4E47B0B1B84F4E474F4E47B0B2B94F4D464F4D464F4D464F4D464F4D464F4D464F4D464F4D46
4F4D464F4D464F4D46B0B2B94F4D464F4D46B1B3BAB1B3BAB1B3BAB1B3BAB1B3BAB1B3BAB1B3BA
B1B3BAB1B3BAB1B3BAB1B3BAB1B3BAB1B3BA4E4C454E4C45

%%PageTrailer
%%Trailer
%%EOF
