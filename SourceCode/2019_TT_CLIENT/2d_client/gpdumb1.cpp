// GPDUMB1.CPP - Game Engine Part I
 
// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  

#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <stdio.h>

#include <d3d9.h>  // directX includes
#include <d3dx9tex.h>  // directX includes
#include "gpdumb1.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// DEFINES ////////////////////////////////////////////////

// TYPES //////////////////////////////////////////////////

// PROTOTYPES /////////////////////////////////////////////

// EXTERNALS /////////////////////////////////////////////

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance

// GLOBALS ////////////////////////////////////////////////
FILE                 *fp_error;            // general error file
LPDIRECT3D9             g_pD3D       = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
LPD3DXSPRITE			g_pSprite;
LPD3DXFONT				g_pFont;

LPDIRECT3DTEXTURE9		g_textures[MAX_TEXTURES];

DWORD                start_clock_count = 0; // used for timing


// these are overwritten globally by DD_Init()
int screen_width  = SCREEN_WIDTH,            // width of screen
    screen_height = SCREEN_HEIGHT,           // height of screen
    screen_bpp    = SCREEN_BPP;              // bits per pixel

// FUNCTIONS //////////////////////////////////////////////

// the BOB engine, note that 90% of the BOB functions have
// the exact same code for the 8 and 16 bit versions, however
// I decided that it was just easier using all 8 or 16 bit 
// versions syntactically, plus you might want to add some
// more functionality to the 16 bit versions, so having separate
// functions makes that easier -- 



///////////////////////////////////////////////////////////

int Create_BOB32(BOB_PTR bob,         // the bob to create
               int x, int y,      // initial posiiton
               int width, int height, // size of bob
               int num_frames,        // number of frames
               int attr)              // attrs
{
// Create the 16-bit BOB object, note that all BOBs 
// are created as offscreen surfaces in VRAM as the
// default, if you want to use system memory then
// set flags equal to DDSCAPS_SYSTEMMEMORY
int index;          // looping var

// set state and attributes of BOB
bob->state          = BOB_STATE_ALIVE;
bob->attr           = attr;
bob->anim_state     = 0;
bob->counter_1      = 0;     
bob->counter_2      = 0;
bob->max_count_1    = 0;
bob->max_count_2    = 0;

bob->curr_frame     = 0;
bob->num_frames     = num_frames;
bob->curr_animation = 0;
bob->anim_counter   = 0;
bob->anim_index     = 0;
bob->anim_count_max = 0; 
bob->x              = x;
bob->y              = y;
bob->xv             = 0;
bob->yv             = 0;

// set dimensions of the new bitmap surface
bob->width  = width;
bob->height = height;
bob->bpp    = 32;

// set all images to null
for (index=0; index < MAX_BOB_FRAMES; index++) {
	bob->tx[index] = bob->ty[index] = 0;
	bob->images[index] = NULL;
}

// set all animations to null
for (index=0; index < MAX_BOB_ANIMATIONS; index++)
    bob->animations[index] = NULL;

// return success
return(1);

} // end Create_BOB32

///////////////////////////////////////////////////////////

int Destroy_BOB32(BOB_PTR bob)
{
// destroy the BOB, simply release the surface

int index; // looping var

// is this bob valid
if (!bob)
    return(0);
   
// bob->images->Release();

// release memory for animation sequences 
for (index=0; index < MAX_BOB_ANIMATIONS; index++)
    if (bob->animations[index])
       free(bob->animations[index]);

// return success
return(1);

} // end Destroy_BOB16

///////////////////////////////////////////////////////////

int Draw_BOB32(BOB_PTR bob)             // bob to draw
{
// draw a bob at the x,y defined in the BOB
// on the destination surface defined in dest

// is this a valid bob
if (!bob)
    return(0);

// is bob visible
if (!(bob->attr & BOB_ATTR_VISIBLE))
   return(1);


	D3DXVECTOR3 pos = D3DXVECTOR3((bob->x - g_left_x) * 65.0f + 8, 
		(bob->y - g_top_y) * 65.0f + 8, 0.0);

	RECT my = {bob->tx[bob->curr_frame], bob->ty[bob->curr_frame], 
		bob->tx[bob->curr_frame] + bob->width, bob->ty[bob->curr_frame] + bob->height };

	g_pSprite->Draw(bob->images[bob->curr_frame], &my, NULL, &pos,
			D3DCOLOR_ARGB(255,255,255,255));

	if (bob->message_time > GetTickCount() - 2000) 
		Draw_Text_D3D(bob->message,static_cast<int>(pos.x),static_cast<int>(pos.y),D3DCOLOR_ARGB(255,200,200,255));


// return success
return(1);
} // end Draw_BOB16


///////////////////////////////////////////////////////////

int Draw_Scaled_BOB32(BOB_PTR bob, int swidth, int sheight)  // bob and new dimensions
{

return(1);

} // end Draw_Scaled_BOB16

int Load_Texture(const wchar_t *fname, int texture_id, int width, int height)
{
D3DXCreateTextureFromFileEx(g_pd3dDevice, fname, width, height, 1, 0,
	 	D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, 		
		D3DX_DEFAULT, D3DX_DEFAULT, 
		D3DCOLOR_ARGB(255, 255, 255, 255), 
		NULL, NULL, &g_textures[texture_id]);
return 1;
}
int Load_Frame_BOB32(BOB_PTR bob, // bob to load with data
                     int texture_id, // bitmap to scan image data from
                     int frame,       // frame to load
                     int cx,int cy,   // cell or absolute pos. to scan image from
                     int mode)        // if 0 then cx,cy is cell position, else 
                                    // cx,cy are absolute coords
{
if (!bob)   return(0);
if (mode==BITMAP_EXTRACT_MODE_CELL)
   {
   bob->tx[frame] = cx*(bob->width+1) + 1;
   bob->ty[frame] = cy*(bob->height+1) + 1;
   } // end if

bob->images[frame] = g_textures[texture_id];
// set state to loaded
bob->attr |= BOB_ATTR_LOADED;
return(1);
} // end Load_Frame_BOB16


////////////////////////////////////////////////////////////////

int Animate_BOB32(BOB_PTR bob)
{
// this function animates a bob, basically it takes a look at
// the attributes of the bob and determines if the bob is 
// a single frame, multiframe, or multi animation, updates
// the counters and frames appropriately

// is this a valid bob
if (!bob)
   return(0);

// test the level of animation
if (bob->attr & BOB_ATTR_SINGLE_FRAME)
    {
    // current frame always = 0
    bob->curr_frame = 0;
    return(1);
    } // end if
else
if (bob->attr & BOB_ATTR_MULTI_FRAME)
   {
   // update the counter and test if its time to increment frame
   if (++bob->anim_counter >= bob->anim_count_max)
      {
      // reset counter
      bob->anim_counter = 0;

      // move to next frame
      if (++bob->curr_frame >= bob->num_frames)
         bob->curr_frame = 0;

      } // end if
  
   } // end elseif
else
if (bob->attr & BOB_ATTR_MULTI_ANIM)
   {
   // this is the most complex of the animations it must look up the
   // next frame in the animation sequence

   // first test if its time to animate
   if (++bob->anim_counter >= bob->anim_count_max)
      {
      // reset counter
      bob->anim_counter = 0;

      // increment the animation frame index
      bob->anim_index++;
      
      // extract the next frame from animation list 
      bob->curr_frame = bob->animations[bob->curr_animation][bob->anim_index];
     
      // is this and end sequence flag -1
      if (bob->curr_frame == -1)
         {
         // test if this is a single shot animation
         if (bob->attr & BOB_ATTR_ANIM_ONE_SHOT)
            {
            // set animation state message to done
            bob->anim_state = BOB_STATE_ANIM_DONE;
            
            // reset frame back one
            bob->anim_index--;

            // extract animation frame
            bob->curr_frame = bob->animations[bob->curr_animation][bob->anim_index];    

            } // end if
        else
           {
           // reset animation index
           bob->anim_index = 0;

           // extract first animation frame
           bob->curr_frame = bob->animations[bob->curr_animation][bob->anim_index];
           } // end else

         }  // end if
      
      } // end if

   } // end elseif

// return success
return(1);

} // end Animate_BOB16

///////////////////////////////////////////////////////////

int Scroll_BOB(void)
{
// this function scrolls a bob 
// not implemented

// return success
return(1);
} // end Scroll_BOB

///////////////////////////////////////////////////////////

int Move_BOB(BOB_PTR bob)
{
// this function moves the bob based on its current velocity
// also, the function test for various motion attributes of the'
// bob and takes the appropriate actions
   

// is this a valid bob
if (!bob)
   return(0);

// translate the bob
bob->x+=bob->xv;
bob->y+=bob->yv;

// test for wrap around
if (bob->attr & BOB_ATTR_WRAPAROUND)
   {
   // test x extents first
   if (bob->x > screen_width)
       bob->x = 0 - bob->width;
   else
   if (bob->x < 0 - bob->width)
       bob->x = screen_width;
   
   // now y extents
   if (bob->y > screen_height)
       bob->y = 0 - bob->height;
   else
   if (bob->y < 0 - bob->height)
       bob->y = screen_height;

   } // end if
else
// test for bounce
if (bob->attr & BOB_ATTR_BOUNCE)
   {
   // test x extents first
   if ((bob->x > screen_width - bob->width) || (bob->x < 0) )
       bob->xv = -bob->xv;
    
   // now y extents 
   if ((bob->y > screen_height - bob->height) || (bob->y < 0) )
       bob->yv = -bob->yv;

   } // end if

// return success
return(1);
} // end Move_BOB

///////////////////////////////////////////////////////////

int Move_BOB16(BOB_PTR bob)
{
// this function moves the bob based on its current velocity
// also, the function test for various motion attributes of the'
// bob and takes the appropriate actions
   

// is this a valid bob
if (!bob)
   return(0);

// translate the bob
bob->x+=bob->xv;
bob->y+=bob->yv;

// test for wrap around
if (bob->attr & BOB_ATTR_WRAPAROUND)
   {
   // test x extents first
   if (bob->x > screen_width)
       bob->x = 0 - bob->width;
   else
   if (bob->x < 0 - bob->width)
       bob->x = screen_width;
   
   // now y extents
   if (bob->y > screen_height)
       bob->y = 0 - bob->height;
   else
   if (bob->y < 0 - bob->height)
       bob->y = screen_height;

   } // end if
else
// test for bounce
if (bob->attr & BOB_ATTR_BOUNCE)
   {
   // test x extents first
   if ((bob->x > screen_width - bob->width) || (bob->x < 0) )
       bob->xv = -bob->xv;
    
   // now y extents 
   if ((bob->y > screen_height - bob->height) || (bob->y < 0) )
       bob->yv = -bob->yv;

   } // end if

// return success
return(1);
} // end Move_BOB16



///////////////////////////////////////////////////////////

int Load_Animation_BOB32(BOB_PTR bob, 
                       int anim_index, 
                       int num_frames, 
                       int *sequence)
{
// this function load an animation sequence for a bob
// the sequence consists of frame indices, the function
// will append a -1 to the end of the list so the display
// software knows when to restart the animation sequence

	int index;
// is this bob valid
if (!bob)
   return(0);

// allocate memory for bob animation
if (!(bob->animations[anim_index] = (int *)malloc((num_frames+1)*sizeof(int))))
   return(0);

// load data into 
for (index=0; index<num_frames; index++)
    bob->animations[anim_index][index] = sequence[index];

// set the end of the list to a -1
bob->animations[anim_index][index] = -1;

// return success
return(1);

} // end Load_Animation_BOB16

///////////////////////////////////////////////////////////

int Set_Pos_BOB32(BOB_PTR bob, int x, int y)
{
// this functions sets the postion of a bob

// is this a valid bob
if (!bob)
   return(0);

// set positin
bob->x = x;
bob->y = y;

// return success
return(1);
} // end Set_Pos_BOB16

///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////

int Set_Anim_Speed_BOB32(BOB_PTR bob,int speed)
{
// this function simply sets the animation speed of a bob
    
// is this a valid bob
if (!bob)
   return(0);

// set speed
bob->anim_count_max = speed;

// return success
return(1);

} // end Set_Anim_Speed16

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

int Set_Animation_BOB32(BOB_PTR bob, int anim_index)
{
// this function sets the animation to play

// is this a valid bob
if (!bob)
   return(0);

// set the animation index
bob->curr_animation = anim_index;

// reset animation 
bob->anim_index = 0;

// return success
return(1);

} // end Set_Animation_BOB16

///////////////////////////////////////////////////////////

int Set_Vel_BOB32(BOB_PTR bob,int xv, int yv)
{
// this function sets the velocity of a bob

// is this a valid bob
if (!bob)
   return(0);

// set velocity
bob->xv = xv;
bob->yv = yv;

// return success
return(1);
} // end Set_Vel_BOB16

///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////

int Hide_BOB32(BOB_PTR bob)
{
// this functions hides bob 

// is this a valid bob
if (!bob)
   return(0);

// reset the visibility bit
RESET_BIT(bob->attr, BOB_ATTR_VISIBLE);

// return success
return(1);
} // end Hide_BOB16

///////////////////////////////////////////////////////////

int Show_BOB32(BOB_PTR bob)
{
// this function shows a bob

// is this a valid bob
if (!bob)
   return(0);

// set the visibility bit
SET_BIT(bob->attr, BOB_ATTR_VISIBLE);

// return success
return(1);
} // end Show_BOB16

///////////////////////////////////////////////////////////

int Collision_BOBS32(BOB_PTR bob1, BOB_PTR bob2)
{
// are these a valid bobs
if (!bob1 || !bob2)
   return(0);

// get the radi of each rect
int width1  = (bob1->width>>1) - (bob1->width>>3);
int height1 = (bob1->height>>1) - (bob1->height>>3);

int width2  = (bob2->width>>1) - (bob2->width>>3);
int height2 = (bob2->height>>1) - (bob2->height>>3);

// compute center of each rect
int cx1 = bob1->x + width1;
int cy1 = bob1->y + height1;

int cx2 = bob2->x + width2;
int cy2 = bob2->y + height2;

// compute deltas
int dx = abs(cx2 - cx1);
int dy = abs(cy2 - cy1);

// test if rects overlap
if (dx < (width1+width2) && dy < (height1+height2))
   return(1);
else
// else no collision
return(0);

} // end Collision_BOBS16

///////////////////////////////////////////////////////////

int DD_Init(int width, int height, int bpp)
{
	if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return 0;

	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
									  main_window_handle,
		                              D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			                          &d3dpp, &g_pd3dDevice ) ) )
		return(0);	// return fail

	D3DXCreateSprite(g_pd3dDevice, &g_pSprite);

	D3DXCreateFont(g_pd3dDevice, 20, 0, FW_BOLD,
		0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		TEXT("±¼¸²"), &g_pFont);


// set globals
screen_height = height;
screen_width = width;
screen_bpp = bpp;

// return success
return(1);
} // end DD_Init

///////////////////////////////////////////////////////////

int DD_Shutdown(void)
{
	for (int i=0; i< MAX_TEXTURES; ++i)
		if (g_textures[i]) g_textures[i]->Release();
	if (g_pFont != NULL)		g_pFont->Release();
	if (g_pSprite != NULL)		g_pSprite->Release();
    if( g_pd3dDevice != NULL)	g_pd3dDevice->Release();
    if( g_pD3D != NULL)			g_pD3D->Release();

// return success
return(1);
} // end DD_Shutdown

///////////////////////////////////////////////////////////   
   
int DD_Flip(void)
{
// this function flip the primary surface with the secondary surface
	g_pd3dDevice->Present (NULL,NULL,NULL,NULL);

// return success
return(1);

} // end DD_Flip

///////////////////////////////////////////////////////////   
   
int DD_Wait_For_Vsync(void)
{
// this function waits for a vertical blank to begin
    
// lpdd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);

// return success
return(1);
} // end DD_Wait_For_Vsync

///////////////////////////////////////////////////////////   
   
int DD_Fill_Surface(D3DCOLOR color)
{
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0 );

// return success
return(1);
} // end DD_Fill_Surface


DWORD Get_Clock(void)
{
// this function returns the current tick count

// return time
return(GetTickCount());

} // end Get_Clock

///////////////////////////////////////////////////////////

DWORD Start_Clock(void)
{
// this function starts the clock, that is, saves the current
// count, use in conjunction with Wait_Clock()

return(start_clock_count = Get_Clock());

} // end Start_Clock

////////////////////////////////////////////////////////////

DWORD Wait_Clock(DWORD count)
{
// this function is used to wait for a specific number of clicks
// since the call to Start_Clock

while((Get_Clock() - start_clock_count) < count);

return(Get_Clock());

} // end Wait_Clock
  
///////////////////////////////////////////////////////////   
   
int Screen_Transition(void)
{
// this function performs a screen transition on the sent
// video buffer
// not implemented


// return success
return(1);
} // end Screen_Transition

///////////////////////////////////////////////////////////

int Draw_Text_D3D(wchar_t *text, int x,int y,D3DCOLOR color)
{
// this function draws the sent text on the sent surface 
// using color index as the color in the palette

	 RECT rectTemp = { x, y, screen_width, screen_height };

	 g_pFont->DrawText(g_pSprite, text, -1, &rectTemp, 0, color);


// return success
return(1);
} // end Draw_Text_GDI

///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

int Create_Bitmap32(BITMAP_IMAGE_PTR image, int x, int y, int width, int height)
{
// this function is used to intialize a bitmap

// allocate the memory
 image->buffer = NULL;

// initialize variables
image->state     = BITMAP_STATE_ALIVE;
image->attr      = 0;
image->width     = width;
image->height    = height;
image->bpp       = 16;
image->x         = x;
image->y         = y;
image->num_bytes = width*height*2;


// return success
return(1);

} // end Create_Bitmap16

///////////////////////////////////////////////////////////////////////////////

int Destroy_Bitmap32(BITMAP_IMAGE_PTR image)
{
// this function releases the memory associated with a bitmap

if (image && image->buffer)
   {
   // free memory and reset vars
   image->buffer->Release();

   // set all vars in structure to 0
   memset(image,0,sizeof(BITMAP_IMAGE));

   // return success
   return(1);

   } // end if
else  // invalid entry pointer of the object wasn't initialized
   return(0);

} // end Destroy_Bitmap


///////////////////////////////////////////////////////////

int Draw_Bitmap32(BITMAP_IMAGE_PTR source_bitmap)
{
// this function draws the bitmap onto the destination memory surface
// if transparent is 1 then color 0 will be transparent
// note this function does NOT clip, so be carefull!!!

	RECT src;
	src.bottom = source_bitmap->y + source_bitmap->height;
	src.left = source_bitmap->x;
	src.top = source_bitmap->y;
	src.right = source_bitmap->x + source_bitmap->width;

if (source_bitmap->attr & BITMAP_ATTR_LOADED)
   {
	D3DXVECTOR3 pos = D3DXVECTOR3(static_cast<float>(source_bitmap->x), static_cast<float>(source_bitmap->y), 0.0);

	g_pSprite->Draw(source_bitmap->buffer, NULL, NULL, &pos,
			D3DCOLOR_ARGB(255,255,255,255));
   // return success
   return(1);

   } // end if
else
   return(0);

} // end Draw_Bitmap16

int Draw_Bitmap32(BITMAP_IMAGE_PTR source_bitmap, int x, int y)
{
// this function draws the bitmap onto the destination memory surface
// if transparent is 1 then color 0 will be transparent
// note this function does NOT clip, so be carefull!!!

	RECT src;
	src.bottom = source_bitmap->y + source_bitmap->height;
	src.left = source_bitmap->x;
	src.top = source_bitmap->y;
	src.right = source_bitmap->x + source_bitmap->width;

if (source_bitmap->attr & BITMAP_ATTR_LOADED)
   {
	D3DXVECTOR3 pos = D3DXVECTOR3(static_cast<float>(x), static_cast<float>(y), 0.0);

	g_pSprite->Draw(source_bitmap->buffer, &src, NULL, &pos,
			D3DCOLOR_ARGB(255,255,255,255));
   // return success
   return(1);

   } // end if
else
   return(0);

} // end Draw_Bitmap16

//////////////////////////////////////////////////////////////////

int Load_Image_Bitmap32(BITMAP_IMAGE_PTR image, // bitmap image to load with data
                      const wchar_t *fname,    // bitmap to scan image data from
                      int cx,int cy,   // cell or absolute pos. to scan image from
                      int mode)        // if 0 then cx,cy is cell position, else 
                                    // cx,cy are absolute coords
{
// this function extracts a bitmap out of a bitmap file

// is this a valid bob
if (!image)
   return(0);

// test the mode of extraction, cell based or absolute
if (mode==BITMAP_EXTRACT_MODE_CELL)
   {
   // re-compute x,y
   cx = cx*(image->width+1) + 1;
   cy = cy*(image->height+1) + 1;
   } // end if

D3DXCreateTextureFromFileEx(g_pd3dDevice, fname, image->width, image->height, 1, 0,
	 	D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, 		
		D3DX_DEFAULT, D3DX_DEFAULT, 
		D3DCOLOR_ARGB(255, 0, 0, 0), 
		NULL, NULL, &image->buffer);


// set state to loaded
image->attr |= BITMAP_ATTR_LOADED;

// return success
return(1);

} // end Load_Image_Bitmap16

///////////////////////////////////////////////////////////

int Scroll_Bitmap(void)
{
// this function scrolls a bitmap
// not implemented

// return success
return(1);
} // end Scroll_Bitmap
///////////////////////////////////////////////////////////

int Copy_Bitmap(void)
{
// this function copies a bitmap from one source to another
// not implemented


// return success
return(1);
} // end Copy_Bitmap
