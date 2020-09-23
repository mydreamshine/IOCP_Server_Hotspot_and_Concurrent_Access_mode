// GPDUMB1.H - Header file for GPDUMB1.CPP game engine library

// watch for multiple inclusions
#ifndef GPDUMB1
#define GPDUMB1

// DEFINES ////////////////////////////////////////////////

// default screen size
#define SCREEN_WIDTH    640  // size of screen
#define SCREEN_HEIGHT   480
#define SCREEN_BPP      32    // bits per pixel


// defines for BOBs
#define BOB_STATE_DEAD         0    // this is a dead bob
#define BOB_STATE_ALIVE        1    // this is a live bob
#define BOB_STATE_DYING        2    // this bob is dying
#define BOB_STATE_ANIM_DONE    1    // done animation state
#define MAX_BOB_FRAMES         64   // maximum number of bob frames
#define MAX_BOB_ANIMATIONS     16   // maximum number of animation sequeces
#define MAX_TEXTURES		   16

#define BOB_ATTR_SINGLE_FRAME   1   // bob has single frame
#define BOB_ATTR_MULTI_FRAME    2   // bob has multiple frames
#define BOB_ATTR_MULTI_ANIM     4   // bob has multiple animations
#define BOB_ATTR_ANIM_ONE_SHOT  8   // bob will perform the animation once
#define BOB_ATTR_VISIBLE        16  // bob is visible
#define BOB_ATTR_BOUNCE         32  // bob bounces off edges
#define BOB_ATTR_WRAPAROUND     64  // bob wraps around edges
#define BOB_ATTR_LOADED         128 // the bob has been loaded

// bitmap defines
#define BITMAP_ID            0x4D42 // universal id for a bitmap
#define BITMAP_STATE_DEAD    0
#define BITMAP_STATE_ALIVE   1
#define BITMAP_STATE_DYING   2 
#define BITMAP_ATTR_LOADED   128

#define BITMAP_EXTRACT_MODE_CELL  0
#define BITMAP_EXTRACT_MODE_ABS   1
// MACROS /////////////////////////////////////////////////

// these read the keyboard asynchronously
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code)   ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// bit manipulation macros
#define SET_BIT(word,bit_flag)   ((word)=((word) | (bit_flag)))
#define RESET_BIT(word,bit_flag) ((word)=((word) & (~bit_flag)))

// initializes a direct draw struct
#define DD_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

// TYPES //////////////////////////////////////////////////

// basic unsigned types
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;


// the blitter object structure BOB
typedef struct BOB_TYP
        {
        int state;          // the state of the object (general)
        int anim_state;     // an animation state variable, up to you
        int attr;           // attributes pertaining to the object (general)
        int x,y;          // position bitmap will be displayed at
        int xv,yv;        // velocity of object
		float scale, rotation;
        int bpp;            // bits per pixel
        int width, height;  // the width and height of the bob
        int width_fill;     // internal, used to force 8*x wide surfaces
        int counter_1;      // general counters
        int counter_2;
        int max_count_1;    // general threshold values;
        int max_count_2;
        int varsI[16];      // stack of 16 integers
        float varsF[16];    // stack of 16 floats
        int curr_frame;     // current animation frame
        int num_frames;     // total number of animation frames
        int curr_animation; // index of current animation
        int anim_counter;   // used to time animation transitions
        int anim_index;     // animation element index
        int anim_count_max; // number of cycles before animation
        int *animations[MAX_BOB_ANIMATIONS]; // animation sequences

		int tx[MAX_BOB_FRAMES],ty[MAX_BOB_FRAMES];

        LPDIRECT3DTEXTURE9 images[MAX_BOB_FRAMES]; // the bitmap images DD surfaces

		WCHAR message[256];
		DWORD message_time;
 
        } BOB, *BOB_PTR;

// the simple bitmap image
typedef struct BITMAP_IMAGE_TYP
        {
        int state;          // state of bitmap
        int attr;           // attributes of bitmap
        int x,y;          // position of bitmap
        int bpp;            // bits per pixel
        int width, height;  // size of bitmap
        int num_bytes;      // total bytes of bitmap
        LPDIRECT3DTEXTURE9 buffer;      // pixels of bitmap
        } BITMAP_IMAGE, *BITMAP_IMAGE_PTR;

// PROTOTYPES /////////////////////////////////////////////

// DirectDraw functions
int DD_Init(int width, int height, int bpp);
int DD_Shutdown(void);
int DD_Flip(void);
int DD_Fill_Surface(D3DCOLOR color);


// 16-bit BOB functions
int Create_BOB32(BOB_PTR bob,int x, int y,int width, int height,int num_frames,int attr);              
int Destroy_BOB32(BOB_PTR bob);
int Draw_BOB32(BOB_PTR bob);
int Draw_Scaled_BOB32(BOB_PTR bob, int swidth, int sheight);
int Load_Texture(const wchar_t *fname, int texture_id, int width, int height);
int Load_Frame_BOB32(BOB_PTR bob, int texture_id, int frame, int cx,int cy,int mode);             
int Animate_BOB32(BOB_PTR bob);
int Scroll_BOB32(void); // ni
int Move_BOB32(BOB_PTR bob);
int Load_Animation_BOB32(BOB_PTR bob, int anim_index, int num_frames, int *sequence);
int Set_Pos_BOB32(BOB_PTR bob, int x, int y);
int Set_Vel_BOB32(BOB_PTR bob,int xv, int yv);
int Set_Anim_Speed_BOB32(BOB_PTR bob,int speed);
int Set_Animation_BOB32(BOB_PTR bob, int anim_index);
int Hide_BOB32(BOB_PTR bob);
int Show_BOB32(BOB_PTR bob);
int Collision_BOBS32(BOB_PTR bob1, BOB_PTR bob2);


// simple 16-bit bitmap image functions
int Create_Bitmap32(BITMAP_IMAGE_PTR image, int x, int y, int width, int height);
int Destroy_Bitmap32(BITMAP_IMAGE_PTR image);
int Draw_Bitmap32(BITMAP_IMAGE_PTR source_bitmap);
int Draw_Bitmap32(BITMAP_IMAGE_PTR source_bitmap, int x, int y);
int Load_Image_Bitmap32(BITMAP_IMAGE_PTR image,const wchar_t *fname,int cx,int cy,int mode);   



// general utility functions
DWORD Get_Clock(void);
DWORD Start_Clock(void);
DWORD Wait_Clock(DWORD count);

// gdi functions
int Draw_Text_D3D(wchar_t *text, int x,int y, D3DCOLOR color);

// GLOBALS ////////////////////////////////////////////////

// GLOBALS ////////////////////////////////////////////////

extern FILE						*fp_error;            // general error file
extern LPDIRECT3D9				g_pD3D; // Used to create the D3DDevice
extern LPDIRECT3DDEVICE9		g_pd3dDevice; // Our rendering device
extern LPD3DXSPRITE				g_pSprite;
extern LPD3DXFONT				g_pFont;

extern LPDIRECT3DTEXTURE9		g_textures[MAX_TEXTURES];

extern DWORD                start_clock_count;    // used for timing


// these are overwritten globally by DD_Init()
extern int screen_width,                            // width of screen
           screen_height,                           // height of screen
           screen_bpp;                              // bits per pixel 

extern int g_left_x, g_top_y;
#endif


