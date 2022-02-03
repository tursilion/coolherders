 rem Generated 7/19/2011 11:53:09 PM by Visual bB Version 1.0.0.554
 rem **********************************
 rem * cool2600.bas
 rem * Cool Herders 2600
 rem * by Tursi - July 2011
 rem * (C) 2011 HarmlessLion LLC
 rem * All rights reserved
 rem Thanks to the AtariAge forums and Random Terrain's documentation efforts!
 rem **********************************

 rem this kernel allows 6 sprites, but missiles are limited. So, just use sprites
 rem sprite 0 (not-multiplexed): sheep (add run animation)
 rem sprite 1 - player 1 (add run animation)
 rem sprite 2 - player 2 (add run animation)
 rem sprite 3 - horizontal lightning sprite (toggles every so many frames for animation)
 rem sprite 4 - vertical lightning sprite (toggles every so many frames for animation)
 rem
 rem My thinking is that lightning runs through a continuous count of 1-6. Frames 1-3 are for player 1,
 rem and reach out from him that range, while 4-6 are for player two. This gives some gaps in the
 rem firing. Shots just knock the other player back or stun the sheep briefly. There is only ever one
 rem lightning bolt onscreen at a time.
 rem 
 rem alternately the default kernel would work, almost. We'd need to define the ball as the sheep ideally,
 rem but neither the ball or missles can have a pattern, so it'd be relatively simple.
 rem
 rem unfortunately, the players will always flicker in this kernel, and if both are shooting at each other, they
 rem will flicker to 20hz. Which I guess isn't that bad. if it is, we can flicker the lightning with the sheep, and
 rem count 1-12 instead of 1-6, with every other frame allocated to the sheep. Makes lightning hard to see though.

 set kernel multisprite
 rem set romsize 8k
 set tv ntsc

 dim p1xd=a
 dim p1yd=b
 dim p2xd=c
 dim p2yd=d
 dim shxd=e
 dim shyd=f
 dim tx=g
 dim ty=h
 dim tmp=i
 dim tmp2=j
 dim pref=k
 dim stx=l
 dim sty=m

 rem set colors (except 0) - these seem acceptable in PAL and SECAM too
 _COLUP1 = $84 : rem blue (virtual sprite)
 COLUP2 = $42 : rem red (virtual sprite)
 COLUP3 = $42 : rem red (virtual sprite)
 COLUP4 = $42 : rem red (virtual sprite)
 COLUPF = $C6 : rem dark green
 COLUBK = $C2 : rem darker green

 rem set playfield size
 pfheight = 7

 rem set sprite options
 NUSIZ0 = $00 : REFP0 = $00 : _NUSIZ1 = $00 : NUSIZ2 = $00 : NUSIZ3=$00 : NUSIZ4=$00
 rem player 0 - REFP0 -no reflection ($08 for reflection - face left)
 rem player 1/2, _NUSIZ1/NUSIZ2: no reflection ($08 for reflection

 rem scores
 scorecolor = 10
 dim sc1 = score
 dim sc2 = score+2
 sc1=$88
 sc2=$88

 rem position everyone (pf multiple is 4 wide by 8 tall)
 player0x = 77 : player0y = 50 : rem player 0 is not flickered, it seems to display 7 pixels later on x, and 2 rows later on Y than the others
 player1x = 28 : player1y = 80 
 player2x = 140 : player2y = 16
 NUSIZ2 = $08
 p1xd=0:p1yd=0:p2xd=0:p2yd=0:shxd=0:shyd=0:stx=0:sty=0

 rem with this kernel, playfield is in ROM
 playfield:
  XXXXXXXXXXXXXXXX
  X...............
  X..XXXXX..XX..XX
  X..X......XX....
  X..X..XX..XX..XX
  X.........XX....
  X..X..XX..XX..XX
  X..X......XX....
  X..XXXXX..XX..XX
  X...............
  XXXXXXXXXXXXXXXX
end

100 rem main loop
 rem must define all graphics each loop

 rem herder 1
 player1:
 %01000100
 %00111000
 %00000000
 %01111100
 %00111110
 %00110000
 %00111100
 %00011000
end
 rem herder 2
 player2:
 %01000100
 %00111000
 %00000000
 %01111100
 %00111110
 %00110000
 %00111100
 %00011000
end
 rem sheep
 player0:
 %00100100
 %01111110
 %11111101
 %11111001
 %11110110
 %01111100
 %00000000
 %00000000
end
 COLUP0 = $0E : rem white 

 rem Sprite Alignment to the playfield
 rem Sprites are 8x8 pixels
 rem The playfield is 32x11, mirrored
 rem A sprite is one playfield row tall, and 2 playfield columns wide
 rem Sprites have a 24 pixel X offset, and an 8 pixel Y offset
 rem so: Sprite->playfield: x=(playerX-24)/4, y=(playerY-8)/8
 rem Sprite 0 (sheep) is offset differently:
 rem Sprite->playfield: x=(player0X-16)/4, y=(player0Y-10)/8
 rem Finally, remember that the Y axis is inverted in this kernel. 0 is at the bottom.
 rem pfread(x,y)=true means the cell is EMPTY and you can move there.

 rem move player 1 in the maze (assuming legal - recheck later)
 player1x=player1x+p1xd : player1y=player1y-p1yd

 if p1xd=1 then _NUSIZ1=0
 if p1xd=255 then _NUSIZ1=8

 rem  *  Converts player's sprite position to playfield position
 tx=(player1x-24)/4
 ty=(player1y-8)/8

 rem check player input
 pref=1
 if p1xd then pref=0
 p1xd=0
 p1yd=0

 if player1x&3 then noupdown1
 if joy0up then p1yd=255
 if joy0down then p1yd=1

noupdown1
 if player1y&7 then goto doublecheck1
 if joy0left then p1xd=255 
 if joy0right then p1xd=1

doublecheck1
 if pref=1 then goto checklrfirst1
 if p1yd=255 then goto pup1
 if p1yd=1 then goto pdown1
checklrfirst1
 pref=0
 if p1xd=255 then goto pleft1
 if p1xd=1 then goto pright1
 if p1yd then goto doublecheck1
 goto movepdone1

 rem  *  Player up
pup1 
 rem  *  Checks for both pieces of a wall above player
 if player1y&7 then goto movepdone1
 tmp=ty+1 : if !pfread(tx,tmp) then p1yd=0 : goto doublecheck1
 tmp2=tx+1 : if !pfread(tmp2, tmp) then p1yd=0 : goto doublecheck1
 p1xd=0
 goto movepdone1

 rem  *  Player down
pdown1
 rem  *  Checks for both pieces of a wall below player
 if player1y&7 then goto movepdone1
 tmp=ty-1 : if !pfread(tx,tmp) then p1yd=0 : goto doublecheck1
 tmp2=tx+1 : if !pfread(tmp2,tmp) then p1yd=0 : goto doublecheck1
 p1xd=0
 goto movepdone1

 rem  *  Player left
pleft1
 rem  *  Checks for a wall to the left of the player
 if player1x&3 then goto movepdone1
 tmp=tx-1 : if !pfread(tmp,ty) then p1xd=0 : goto doublecheck1
 p1yd=0
 goto movepdone1

 rem  *  Player right
pright1
 rem  *  Checks for a wall to the right of the player
 if player1x&3 then goto movepdone1
 tmp=tx+2 : if !pfread(tmp,ty) then p1xd=0 : goto doublecheck1
 p1yd=0

movepdone1
 
 rem Now move player 2 the same way
 player2x=player2x+p2xd : player2y=player2y-p2yd
 if p2xd=1 then NUSIZ2=0
 if p2xd=255 then NUSIZ2=8

 rem  *  Converts player's sprite position to playfield position
 tx=(player2x-24)/4
 ty=(player2y-8)/8

 rem check player input
 pref=1
 if p2xd then pref=0
 p2xd=0
 p2yd=0

 if player2x&3 then noupdown2
 if joy1up then p2yd=255
 if joy1down then p2yd=1

noupdown2
 if player2y&7 then goto doublecheck2
 if joy1left then p2xd=255 
 if joy1right then p2xd=1

doublecheck2
 if pref=1 then goto checklrfirst2
 if p2yd=255 then goto pup2
 if p2yd=1 then goto pdown2
checklrfirst2
 pref=0
 if p2xd=255 then goto pleft2
 if p2xd=1 then goto pright2
 if p2yd then goto doublecheck2
 goto movepdone2

 rem  *  Player up
pup2 
 rem  *  Checks for both pieces of a wall above player
 if player2y&7 then goto movepdone2
 tmp=ty+1 : if !pfread(tx,tmp) then p2yd=0 : goto doublecheck2
 tmp2=tx+1 : if !pfread(tmp2, tmp) then p2yd=0 : goto doublecheck2
 p2xd=0
 goto movepdone2

 rem  *  Player down
pdown2
 rem  *  Checks for both pieces of a wall below player
 if player2y&7 then goto movepdone2
 tmp=ty-1 : if !pfread(tx,tmp) then p2yd=0 : goto doublecheck2
 tmp2=tx+1 : if !pfread(tmp2,tmp) then p2yd=0 : goto doublecheck2
 p2xd=0
 goto movepdone2

 rem  *  Player left
pleft2
 rem  *  Checks for a wall to the left of the player
 if player2x&3 then goto movepdone2
 tmp=tx-1 : if !pfread(tmp,ty) then p2xd=0 : goto doublecheck2
 p2yd=0
 goto movepdone2

 rem  *  Player right
pright2
 rem  *  Checks for a wall to the right of the player
 if player2x&3 then goto movepdone2
 tmp=tx+2 : if !pfread(tmp,ty) then p2xd=0 : goto doublecheck2
 p2yd=0

movepdone2

 rem and the sheep?
 tmp=shxd | shyd
 if tmp then goto processmove
 rem choose new sheep target (128x88) - don't move this frame
 stx = rand&127
 sty = (rand&63) + 12
 goto parsenewsheep

processmove
 player0x=player0x+shxd : player0y=player0y-shyd
 if shxd=1 then REFP0=0
 if shxd=255 then REFP0=8

 pref=1
 if shxd then pref=0
 shxd=0
 shyd=0

parsenewsheep
 rem  *  Converts player's sprite position to playfield position
 tx=(player0x-16)/4
 ty=(player0y-10)/8

 if player0x&3 then noupdown3
 tmp=player0y-sty
 if tmp > 0 then shyd=1
 if tmp > 128 then shyd=255

noupdown3
 tmp=player0y-2
 if tmp&7 then goto doublecheck3
 tmp=stx-player0x
 if tmp > 0 then shxd=1
 if tmp > 128 then shxd = 255

doublecheck3
 if pref=1 then goto checklrfirst3
 if shyd=255 then goto pup3
 if shyd=1 then goto pdown3
checklrfirst3
 pref=0
 if shxd=255 then goto pleft3
 if shxd=1 then goto pright3
 if shyd then goto doublecheck3
 goto movepdone3

 rem  *  Player up
pup3
 rem  *  Checks for both pieces of a wall above player
 tmp=player0y-2
 if tmp&7 then goto movepdone3
 tmp=ty+1 : if !pfread(tx,tmp) then shyd=0 : goto doublecheck3
 tmp2=tx+1 : if !pfread(tmp2, tmp) then shyd=0 : goto doublecheck3
 shxd=0
 goto movepdone3

 rem  *  Player down
pdown3
 rem  *  Checks for both pieces of a wall below player
 tmp=player0y-2
 if tmp&7 then goto movepdone3
 tmp=ty-1 : if !pfread(tx,tmp) then shyd=0 : goto doublecheck3
 tmp2=tx+1 : if !pfread(tmp2,tmp) then shyd=0 : goto doublecheck3
 shxd=0
 goto movepdone3

 rem  *  Player left
pleft3
 rem  *  Checks for a wall to the left of the player
 if player0x&3 then goto movepdone3
 tmp=tx-1 : if !pfread(tmp,ty) then shxd=0 : goto doublecheck3
 shyd=0
 goto movepdone3

 rem  *  Player right
pright3
 rem  *  Checks for a wall to the right of the player
 if player0x&3 then goto movepdone3
 tmp=tx+2 : if !pfread(tmp,ty) then shxd=0 : goto doublecheck3
 shyd=0

movepdone3


 drawscreen
 goto 100



 inline pfread_msk.asm

