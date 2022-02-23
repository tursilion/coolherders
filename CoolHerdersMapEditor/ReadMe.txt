Starting up:
-To start, select a folder to work in. There must be at least one file in that folder
 as you need to select a file - any kind of file - to start.
-The program will automatically import graphics and level data. It is expected that
 there is only ONE stage per folder.
-PNGs are imported as 256x256 tile pages with the name "levelxyz.png"
 x = the level number, any number from 0-9 (editor doesn't care)
 y = the tile page, a, b, or c (with c being reserved for destructibles)
 z = the animation frame from 1-4. If this character is absent, the page is loaded into all four frames
-The level map is imported from a text file named "levelx.txt", x meaning the same as above

Using:
-You will see the three tile pages down the left side, and the full map across the top of the rest.
 It will all be animating at about 4 fps.
-Change tile pages completely by dragging a new 256x256x32bit PNG to the page you wish to replace.
 A box will pop up asking which frame you wish to add it to. You may use the mouse or keyboard to select 1-4
-You may add individual tiles of 32x32 or 48x48 pixels by dragging them to the appropriate tile. Again, you
 will be asked which animation frame to drop the file in.
-When you close, the files are all saved, and backups made of the files already on the disk
-To draw on the map, first select a tile. It will be outlined. You may draw on the map by clicking, dragging,
 and releasing. The tiles you draw will not appear until after you let go, but all the points you visit will
 be updated!
-If you draw with breakables, they are placed *on top* of the existing tile.
-The current animation frame is shown cycling under the map. You can stop the animation by deselecting 'Animate'
 and choosing the frame you want. When a specific frame is selected, drag-and-drop of files will automatically
 affect only that frame.
-The Undo can undo the last change made to the map (only 1 level), not to tile pages
-The terrain setting lets you set the 3d/impassible/flat flags. Select one, and the map will light up all cells
 that have that setting. Any cell you click gets the new setting (regardless of tile!). Flat is normal flat passable
 areas (ie: floors, trails). Impassible is drawn flat but blocks the player (mostly the outside barrier). 3D is
 drawn overlapping and blocks the player (pretty much everything on the playfield)
-Select 'Show Safe', and the border areas will be highlighted with red. There should be no player area there, 
 just walls. Also, the player and sheep starts will be highlighted with blue. Be sure those areas are clear
 and that sheep have paths to scatter on!
-Select 'Hide Breakable', and breakable objects will not be shown. They will still be saved!

Tips:
-Always work from copies to avoid corrupting your original art. Better still, build the tile pages right
 in the map editor from your individual tiles!
-You can add a full animation sequence to a tile - select all four tiles and drag them to the correct place
 on the dialog. Each one will cause the animation frame dialog to pop up, and you can quickly select the correct
 frame for each (I recommend reading the filenames just in case they don't get added in the order you expect!)
-You can set a flat/impassible/3d property on any tile by clicking it while in the appropriate mode. There is no
 visual feedback, but all new tiles drawn from that one will have the new property. This hidden selection is saved
 between sessions! 
-Watch the borders! Use the coordinates that appear beside the animation frame buttons when you're on the map.
 The outer egde is truncated to show what the Dreamcast really draws, but it runs from 0-20 across, and 0-15 down.
-Likewise, watch the safe boundaries! We have decided that the top boundary is 3 tiles, the bottom is 2, and the
 left and right edges are 2 each. The 2 is for TV overscan, the extra 1 at the top is for the score bars.
-Remeber that page 3 is currently reserved for explosives - the main item must be the first one in the row, and
 we can have up to five styles. If that's going to be a problem, I can change it, but until we need to let's not :)
-When you want to quit normally, just click the 'X' at top right. All your changes will be saved.
-The backup is created per session - if you save 50 times, the backup is still the files that were there when you
 started.
-This program does NOT accurately handle the 3D flags! Make sure they're set right as overlap of flat tiles is
 not guaranteed on the Dreamcast.
