/* graphlibrary.h */

#ifndef GRAPHLIBRARY_H
#define GRAPHLIBRARY_H

#include "vcfw/rtos/rtos.h"   
#include "vcfw/rtos/common/rtos_common_mem.h"


/* structure represents a coordiante on the screen */
struct s_coordinate{
 uint32_t x;
 uint32_t y;
};
typedef struct s_coordinate t_Coordinate;


typedef enum{INTEGER,STRING,REAL}  t_DataType;
typedef enum{NO_ERROR, DATA_ERROR, SIZE_ERROR, MALLOC_ERROR,NO_FB_SET,NO_SETTINGS} t_Return;

/* structure represents color offset */
struct s_colorOffset{
 int16_t redOffset; // this offset is added to background color - transparency
 int16_t greenOffset;
 int16_t blueOffset;
};
typedef struct s_colorOffset t_ColorOffset;

typedef uint16_t t_Color;
typedef uint32_t t_Radius;
typedef uint32_t t_Width;
typedef uint32_t t_Angle; // represents an angle in degrees from the range 0-360

/* structure represents size / 2D object dimensions */
struct s_size{
 uint32_t height; // in pixels
 uint32_t width;  // in pixels
};
typedef struct s_size t_Size;


/* structure represents general graph settings */
struct s_graphSettings {

 /* COMMON GRAPH DETAILS */
 t_Color trBgCol;                      // color of a rectangle in the background
 uint8_t leftMargin;                   // left margin
 uint8_t rightMargin;                  // right margin
 uint8_t topMargin;                    // top margin
 uint8_t buttomMargin;                 // buttom margin
 t_Color textColor;                    // text color
 t_Color textOutlineColor;             // text outline color
 t_Color *colors;                      // array of colors used in graphs (for lines/bars/circle segments)
 uint8_t label2GraphDist;              // distance from labels to graph
 uint8_t legend2GraphDist;             // legend to graph distance
 
 /*PIE CHART SPECIFIC*/
 t_Width outlineWidth;                 // circle outline width
 t_Width insideLineWidth;              // width of lines separating segments
 t_Color lineColor;                    // color of lines separating segments
 uint8_t smallCircleRadius;            // radius of the circle in the center



  uint8_t floatPrecision;              // float precision (number decimal digits)

 /*LINE&BAR*/
 t_Width lineWidthInsideGraph;         // width of grid lines
 t_Color lineColorInsideGraph;         // color of grid lines
 t_Color bgColorInsideGraph;           // graph background grid color
 uint8_t legendEnable;                 // legend enable flag

 /*LINE*/
 t_Width curveWidth;                   // data curve width

 /*BAR*/
 t_Width barOutlineWidth;              // bar outline width
 t_ColorOffset barOutLineColorOffset;  //color offset of bar outline
 uint8_t maxBarWidth;                  // maximum bar width

 /*TABLE*/
 uint8_t lineWidthInTable;             // width of lines inside table
 t_Color lineColorInTable;             // color of lines inside table
 uint8_t rowHeight;                    // hight of rows
 uint8_t rowWidth;                     // width of rows
 uint8_t linesEnabled;                 // line enable flag

};
typedef struct s_graphSettings t_GraphSettings;



/*--PIE structures--*/

struct s_PieData{
 char *itemName;                      // name used in the legend
 float value;                         // value
};
typedef struct s_PieData t_PieData;

/* structure represents pie chart details */
struct s_PieDetails{
 t_Coordinate coordinate;             // top-left coordiante
 t_Size size;                         // dimensions
 t_GraphSettings *gs;                 // graph settings
 char *graphTitle;                    // main title
 uint32_t nrOfItems;                  // number of data items
 t_PieData *data;                     // array of data items
};
typedef struct s_PieDetails t_PieDetails;



/*--LINE structures--*/

struct s_DataPoint{
 float value;                         // y-value of a datapoint
};
typedef struct s_DataPoint t_DataPoint;

/*structure represents a curve*/
struct s_Curve{
 char* itemName;                      // name of a curve used in the legend
 uint32_t nrOfPoints;                 // number of data points
 t_DataPoint *points;                 // array of ordered data points
};
typedef struct s_Curve t_Curve;

/* structure represents line graph details */
struct s_LineDetails{
 t_Coordinate coordinate;             // top-left coordiante
 t_Size size;                         // dimensions
 t_GraphSettings *gs;                 // graph settings  
 char *graphTitle;                    // main title
 char* x_title;                       // x-axis title
 char* y_title;                       // y-axis title
 uint8_t autoscaleX;                  // flag that denotes that x axis will(1) or not(0) be auto-scaled/adjusted
 struct {
         uint32_t max;                // max x-axis value
         uint32_t incSteps;           // specifies where x-axis labels are drawn (e.g. 50 means labels are drawn every 50 x-units)
        } xScale;                     // if autoscaleX is not set this parameter is used to format the graph 
 uint8_t autoscaleY;                  // flag that denotes that y axis will(1) or not(0) be auto-scaled
 struct {
         float max;                   // max y-axis value
         float incSteps;              // specifies where y-axis labels are drawn (e.g. 50 means labels are drawn every 50 y-units)
        } yScale;                     // if autoscaleY is not set this parameter is used to format the graph 
 uint8_t nrOfCurves;
 t_Curve *curves;
};
typedef struct s_LineDetails t_LineDetails;


/*--BAR structures--*/

struct s_BarData{
 float value; // bar value
};
typedef struct s_BarData t_BarData;

struct s_BarSet{
 char* itemName;  // for the legend
 t_BarData *bars; // array of bars (these are drawn separately with a gap in between)
};
typedef struct s_BarSet t_BarSet;

/* structure represents line graph details */
struct s_BarDetails{
 t_Coordinate coordinate;             // top-left coordiante
 t_Size size;                         // dimensions
 t_GraphSettings *gs;                 // graph settings 
 char *graphTitle;                    // main title
 char* x_title;                       // x-axis title
 char* y_title;                       // y-axis title
 uint8_t autoscaleY;                  // flag that denotes that y axis will(1) or not(0) be auto-scaled
 struct {
         float max;                   // max y-axis value
         float incSteps;              // specifies where y-axis labels are drawn (e.g. 50 means labels are drawn every 50 y-units)
        } yScale;                     // if autoscaleY is not set this parameter is used to format the graph                    
 int nrOfBarsInSet;                   // number of bar sets drawn
 char **xbarTitles;                   // array of bar set names (drawn on x axis)
 int nrOfSets;                        // number of bars drawn together (i.e. w/o a gap in between)
 t_BarSet *barSets;                   // data bar sets
};
typedef struct s_BarDetails t_BarDetails;


/*--TABLE structures--*/

struct s_TableData {
 int column;                          // specifies column
 int row;                             // specified row
 t_DataType type;                     // item type
 union {char *str; int i; float f;} value; // item value
};

typedef struct s_TableData t_TableData;

/* structure represents table details */
struct s_TableDetails{
 t_Coordinate coordinate;             // top-left coordiante
 t_Size size;                         // dimensions
 t_GraphSettings *gs;                 // graph settings 
 char *graphTitle;                    // main title
 int nrOfColumns;                     // number of columns
 int nrOfRows;                        // number of rows
 char **columnHeadings;               // column headings
 char **rowHeadings;                  // row headings
 t_TableData *data;                   // data
};
typedef struct s_TableDetails t_TableDetails;



/*structure representing frame buffer/an image*/
struct s_image{
 uint32_t width;
 uint32_t height;
 t_Color *image_data;                 // pixel values
};
typedef struct s_image t_Image;


//--------------------------------------------------------------------------------------------------

/*FUNCTION DECLARATIONS*/

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void setFB(t_Image*);

   DESCRIPTION: 
    Sets a frame buffer in the graph library. Must be be always called
    prior using the library.

   ARGUMENT(S): 
    t_Image* - pointer to frame buffer

   RETURNED VALUE: -

   SIDE EFFECTS: 
    The pointer to frame buffer is set in graphlibrary.c.
   ------------------------------------------------------------------*/
void setFB(t_Image*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    t_Return pieChart(t_PieDetails*);

   DESCRIPTION: 
    Draws a pie chart.

   ARGUMENT(S): 
    t_PieDetails* - pointer to data and settings required to draw a pie
    chart

   RETURNED VALUE: 
    t_Return - errorcodes:
       
       NO_ERROR      - upon successful completion 
       DATA_ERROR    - data error
       SIZE_ERROR    - graph size error(either too small or too big)
       MALLOC_ERROR  - memory alllocation failed
       NO_FB_SET     - no frame buffer set
       NO_SETTINGS   - settings not set

   SIDE EFFECTS: 
    A pie chart is drawn into frame buffer.
   ------------------------------------------------------------------*/
t_Return pieChart(t_PieDetails*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    t_Return barGraph(t_BarDetails*);

   DESCRIPTION: 
    Draws a bar graph.

   ARGUMENT(S): 
    t_BarDetails* - pointer to data and settings required to draw a bar
    graph

   RETURNED VALUE: 
    t_Return - errorcodes:
       
       NO_ERROR      - upon successful completion 
       DATA_ERROR    - data error
       SIZE_ERROR    - graph size error(either too small or too big)
       MALLOC_ERROR  - memory alllocation failed
       NO_FB_SET     - no frame buffer set
       NO_SETTINGS   - settings not set

   SIDE EFFECTS: 
    A bar graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
t_Return barGraph(t_BarDetails*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    t_Return lineGraph(t_LineDetails*);

   DESCRIPTION: 
    Draws a line graph.

   ARGUMENT(S): 
    t_BarDetails* - pointer to data and settings required to draw a line
    graph

   RETURNED VALUE: 
    t_Return - errorcodes:
       
       NO_ERROR      - upon successful completion 
       DATA_ERROR    - data error
       SIZE_ERROR    - graph size error(either too small or too big)
       MALLOC_ERROR  - memory alllocation failed
       NO_FB_SET     - no frame buffer set
       NO_SETTINGS   - settings not set

   SIDE EFFECTS: 
    A line graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
t_Return lineGraph(t_LineDetails*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    t_Return tableGraph(t_TableDetails*);

   DESCRIPTION: 
    Draws a data table.

   ARGUMENT(S): 
    t_TableDetails* - pointer to data and settings required to draw a data
    table

   RETURNED VALUE: 
    t_Return - errorcodes:
       
       NO_ERROR      - upon successful completion 
       DATA_ERROR    - data error
       SIZE_ERROR    - graph size error(either too small or too big)
       MALLOC_ERROR  - memory alllocation failed
       NO_FB_SET     - no frame buffer set
       NO_SETTINGS   - settings not set

   SIDE EFFECTS: 
    A data table is drawn into frame buffer.
   ------------------------------------------------------------------*/
t_Return tableGraph(t_TableDetails*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawLine(uint32_t,uint32_t,uint32_t,uint32_t,t_Color,t_Width);

   DESCRIPTION: 
    Draws a straight anti-aliased line between two points.

   ARGUMENT(S): 
    uint32_t - x coordinate of the first point
    uint32_t - y coordinate of the first point
    uint32_t - x coordinate of the second point
    uint32_t - y coordinate of the second point
    t_Color  - line color 
    t_Width  - line width

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A line is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawLine(uint32_t,uint32_t,uint32_t,uint32_t,t_Color,t_Width);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawRectOutline(uint32_t,uint32_t,uint32_t,uint32_t,t_Color,t_Width);

   DESCRIPTION: 
    Draws a rectangle outline.

   ARGUMENT(S): 
    uint32_t - x coordinate of the top-left corner
    uint32_t - y coordinate of the top-left corner
    uint32_t - width
    uint32_t - height
    t_Color  - color 
    t_Width  - outline width

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A rectangle outline is drawn in the destination image.
   ------------------------------------------------------------------*/
void drawRectOutline(uint32_t,uint32_t,uint32_t,uint32_t,t_Color,t_Width);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void printText(const char*,uint32_t,uint32_t,t_Color,t_Color);

   DESCRIPTION: 
    Draws a line of text.

   ARGUMENT(S): 
    const char* - a string of symbols(no special characters supported)
    uint32_t    - x coordinate of the top left corner
    uint32_t    - y coordinate of the top left corner
    t_Color     - text color
    t_Color     - text outline color

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A line of text is drawn into frame buffer.
   ------------------------------------------------------------------*/
void printText(const char*,uint32_t,uint32_t,t_Color,t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    int int2String(char*,int32_t);

   DESCRIPTION: 
    Converts an integer into a string.

   ARGUMENT(S): 
    char*   - destination string
    int32_t - integer number

   RETURNED VALUE: 
    int - length of the resulting string
    
   SIDE EFFECTS: 
    Destination location is modified.
   ------------------------------------------------------------------*/
int int2String(char*,int32_t);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    int float2String(char*,float,uint8_t);

   DESCRIPTION: 
    Converts a floating point number into a string.

   ARGUMENT(S): 
    char*   - destination location
    float   - floating point number
    uint8_t - number of decimal digits

   RETURNED VALUE: 
    int - length of the resulting string
    
   SIDE EFFECTS: 
    Destination location is modified.
   ------------------------------------------------------------------*/
int float2String(char*,float,uint8_t);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    t_Color getColor(uint16_t, uint16_t, uint16_t);

   DESCRIPTION: 
    Returns a solid color consisting of specified RGB components.

   ARGUMENT(S): 
    uint16_t - RED
    uint16_t - GREEN
    uint16_t - BLUE

   RETURNED VALUE: 
    t_Color - resulting color
    
   SIDE EFFECTS: -
   ------------------------------------------------------------------*/
t_Color getColor(uint16_t, uint16_t, uint16_t);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawCircleOutline(uint32_t, uint32_t,t_Radius, t_Color,t_Width);

   DESCRIPTION: 
    Draws a circle outline.

   ARGUMENT(S): 
    uint32_t - x coordiante of the center of a circle
    uint32_t - y coordiante of the center of a circle
    t_Radius - radius
    t_Width  - outline width


   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    A circle outline is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawCircleOutline(uint32_t, uint32_t,t_Radius, t_Color,t_Width);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawSolidCircle(t_Coordinate,t_Radius,t_Color);

   DESCRIPTION: 
    Draws a filled circle.

   ARGUMENT(S): 
    t_Coordinate - coordinate of the center of a circle
    t_Radius     - radius
    t_Color      - color

   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    A circle is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawSolidCircle(t_Coordinate,t_Radius,t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawSolidSegment(t_Coordinate,t_Radius,t_Angle,t_Angle,t_Color);

   DESCRIPTION: 
    Draws a filled circle segment.

   ARGUMENT(S): 
    t_Coordinate - coordinate of the center of a circle
    t_Radius     - radius
    t_Angle      - start angle(<360)
    t_Angle      - end angle(<=360)
    t_Color      - color

   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    A circle segment is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawSolidSegment(t_Coordinate,t_Radius,t_Angle,t_Angle,t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawRect(uint32_t,uint32_t,uint32_t,uint32_t, t_Color);

   DESCRIPTION: 
    Draws a filled rectangle.

   ARGUMENT(S): 
    uint32_t - x coordinate of the top left corner
    uint32_t - y coordinate of the top left corner
    uint32_t - width
    uint32_t - height
    t_Color  - color

   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    A rectangle is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawRect(uint32_t,uint32_t,uint32_t,uint32_t, t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void defaultSettings(t_GraphSettings*);

   DESCRIPTION: 
    Retrieves default graph settings.

   ARGUMENT(S): 
    t_GraphSettings* - destination

   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    Argument is filled with default graph settings.
   ------------------------------------------------------------------*/
void defaultSettings(t_GraphSettings*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void cleanUp();

   DESCRIPTION: 
    Deallocates memory on the heap. Called when library will not be used
    any more.

   ARGUMENT(S): 
    t_GraphSettings* - destination

   RETURNED VALUE: -
    
   SIDE EFFECTS: 
    Deallocates memory on the heap.
   ------------------------------------------------------------------*/
void cleanUp();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    uint16_t getRed(t_Color);

   DESCRIPTION: 
    Retrieves red component of supplied color.

   ARGUMENT(S): 
    t_Color - color

   RETURNED VALUE: 
    t_Color - red component
    
   SIDE EFFECTS: -
   ------------------------------------------------------------------*/
uint16_t getRed(t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    uint16_t getGreen(t_Color);

   DESCRIPTION: 
    Retrieves green component of supplied color.

   ARGUMENT(S): 
    t_Color - color

   RETURNED VALUE: 
    t_Color - green component
    
   SIDE EFFECTS: -
   ------------------------------------------------------------------*/
uint16_t getGreen(t_Color);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    uint16_t getBlue(t_Color);

   DESCRIPTION: 
    Retrieves blue component of supplied color.

   ARGUMENT(S): 
    t_Color - color

   RETURNED VALUE: 
    t_Color - blue component
    
   SIDE EFFECTS: -
   ------------------------------------------------------------------*/
uint16_t getBlue(t_Color);



#endif
