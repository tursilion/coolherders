//-----------------------------------------------------------------------------
// <copyright file="ElectricZapper.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using CoolHerders.GameItems;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;

    /// <summary>
    /// This class handles the player's sheep-stunner
    /// </summary>
    public class ElectricZapper
    {
        /// <summary>
        /// The current length of the zapper output
        /// </summary>
        public int CurrentLength;

        /// <summary>
        /// The charged power of the zapper 0-400
        /// </summary>
        public int ZapperPower;

        /// <summary>
        /// The maximum zapper power that this player can currently reach
        /// </summary>
        public int MaxZapperPower;

        /// <summary>
        /// A ray which will be used to represent the zapper for collision purposes
        /// </summary>
        public Ray CollisionRay;

        /// <summary>
        /// Is the zapper actually firing
        /// </summary>
        public bool ZapperOn = false;

        /// <summary>
        /// If the beam has hit something, records the column of the impact
        /// </summary>
        public int BeamHitItemColumn;

        /// <summary>
        /// If the beam has hit something, records the column of the impact
        /// </summary>
        public int BeamHitItemRow;

        /// <summary>
        /// The locations (offset from the player position) at which we might draw a zapper
        /// </summary>
        private static readonly Rectangle[] spriteLocations = new Rectangle[]
        { 
            new Rectangle(48 * 3, 48 * 2, 48, 48), new Rectangle(48 * 3, 48 * 3, 48, 48),
            new Rectangle(48 * 0, 48 * 3, 48, 48), new Rectangle(48 * 1, 48 * 3, 48, 48),
            new Rectangle(48 * 3, 48 * 2, 48, 48), new Rectangle(48 * 3, 48 * 3, 48, 48),
            new Rectangle(48 * 0, 48 * 3, 48, 48), new Rectangle(48 * 1, 48 * 3, 48, 48),
            new Rectangle(48 * 2, 48 * 2, 48, 48), new Rectangle(48 * 2, 48 * 3, 48, 48),
            new Rectangle(48 * 0, 48 * 2, 48, 48), new Rectangle(48 * 1, 48 * 2, 48, 48),
            new Rectangle(48 * 2, 48 * 2, 48, 48), new Rectangle(48 * 2, 48 * 3, 48, 48),
            new Rectangle(48 * 0, 48 * 2, 48, 48), new Rectangle(48 * 1, 48 * 2, 48, 48),
        };

        /// <summary>
        /// The amount of time that the player fires a segment for
        /// </summary>
        private static readonly TimeSpan fireDuration = TimeSpan.FromMilliseconds(5);

        /// <summary>
        /// The amount of time between spark effect flips
        /// </summary>
        private static readonly TimeSpan flipDuration = TimeSpan.FromMilliseconds(75);

        /// <summary>
        /// The player holding this zapper
        /// </summary>
        private PlayerCharacter holdingPlayer;

        /// <summary>
        /// The time which the zapper will fire for
        /// </summary>
        private TimeSpan timeFiring;

        /// <summary>
        /// The time before the sprite needs to flip
        /// </summary>
        private TimeSpan timeToFlip;

        /// <summary>
        /// Is the oscillation flipped
        /// </summary>
        private bool flipDirection;

        /// <summary>
        /// Parent game component
        /// </summary>
        private MazeGameComponent parentComponent;

        /// <summary>
        /// The sprite effects that need to be applied during the draw
        /// </summary>
        private SpriteEffects spriteEffects;

        /// <summary>
        /// The position from which to start drawing the zapper
        /// </summary>
        private Vector2 zapperPosition;

        /// <summary>
        /// The depth to draw the zapper at
        /// </summary>
        private float zapperDepth;

        /// <summary>
        /// The increment to move in the X direction while drawing each zapper unit
        /// </summary>
        private int incrementX;

        /// <summary>
        /// The increment to move in the Y direction while drawing each zapper unit
        /// </summary>
        private int incrementY;

        /// <summary>
        /// This holds a reference to the texture that has all our truly miscellaneous items on it.
        /// It is critical that this be a shared reference, as this is going to be used all over the place
        /// </summary>
        private Texture2D miscItemsTexture;

        /// <summary>
        /// The origin from which to draw the graphics.
        /// This is needed because we have to center things on the player
        /// </summary>
        private Vector2 currentOrigin;

        /// <summary>
        /// Initializes a new instance of the ElectricZapper class
        /// </summary>
        /// <param name="parentComponent">The parent component on which this zapper will be drawn</param>
        /// <param name="holdingPlayer">The player holding this zapper</param>
        internal ElectricZapper(MazeGameComponent parentComponent, PlayerCharacter holdingPlayer)
        {
            this.miscItemsTexture = parentComponent.PlayGameContent.Load<Texture2D>("MiscGraphics\\powerupsetc");
            this.parentComponent = parentComponent;
            this.holdingPlayer = holdingPlayer;
            this.MaxZapperPower = 200;
        }

        /// <summary>
        /// Cause the player's zapper to begin firing
        /// </summary>
        public void FireZapper()
        {
            this.timeFiring = fireDuration;
            this.timeToFlip = flipDuration;
            this.ZapperOn = true;
        }

        /// <summary>
        /// Cause the player's zapper to stop firing
        /// </summary>
        public void StopFiring()
        {
            this.CurrentLength = 0;
            this.timeFiring = TimeSpan.Zero;
            this.timeToFlip = TimeSpan.Zero;
            this.ZapperOn = false;
        }

        /// <summary>
        /// Updates the player's zapper within the game world
        /// </summary>
        /// <param name="gameTime">The current GameTime structure for this game</param>
        public void Update(GameTime gameTime)
        {
            if (this.ZapperOn)
            {
                this.timeFiring -= gameTime.ElapsedGameTime;
                if (this.timeFiring.Ticks <= 0)
                {
                    if (this.ZapperPower > 0)
                    {
                        this.ZapperPower -= 5;
                    }
                    else
                    {
                        this.ZapperPower = 0;
                    }

                    if (this.ZapperPower == 0)
                    {
                        this.timeFiring = TimeSpan.Zero;
                    }
                    else
                    {
                        this.timeFiring += fireDuration;
                    }
                }

                this.CurrentLength = this.ZapperPowerToTiles();
            }
            else
            {
                this.timeFiring -= gameTime.ElapsedGameTime;
                if (this.timeFiring.Ticks <= 0)
                {
                    if (this.ZapperPower < this.MaxZapperPower)
                    {
                        this.ZapperPower += 6 - (this.holdingPlayer.NumberOfSheep / 5);
                    }

                    if (this.ZapperPower == this.MaxZapperPower)
                    {
                        this.timeFiring = TimeSpan.Zero;
                    }
                    else
                    {
                        this.timeFiring += fireDuration;
                    }
                }
            }

            int maxLength = this.FindMaxLength(false);
            if (this.CurrentLength > maxLength)
            {
                this.CurrentLength = maxLength;
            }

            this.timeToFlip -= gameTime.ElapsedGameTime;
            if (this.timeToFlip.Ticks <= 0)
            {
                this.timeToFlip = flipDuration;
                this.flipDirection = !this.flipDirection;
            }

            int playerCol = (int)this.holdingPlayer.CharacterPosition.X / 32;
            int playerRow = (int)this.holdingPlayer.CharacterPosition.Y / 32;
            this.zapperDepth = ((playerRow - 1) * 0.05f) + ((playerCol + 1) * 0.002f) + 0.002f;
            this.incrementX = 0;
            this.incrementY = 0;

            this.zapperPosition = Vector2.Zero;
            this.spriteEffects = SpriteEffects.None;

            this.CollisionRay.Position = new Vector3(this.holdingPlayer.CharacterPosition, 0.0f);
            
            switch (this.holdingPlayer.LastDirection)
            {
                case Direction.DirectLeft:
                    this.zapperPosition.X = this.holdingPlayer.CharacterPosition.X + this.holdingPlayer.DrawingOffset.X;
                    this.zapperPosition.Y = this.holdingPlayer.CharacterPosition.Y + this.holdingPlayer.DrawingOffset.Y;
                    this.currentOrigin = new Vector2(32.0f, 16.0f);
                    this.spriteEffects = SpriteEffects.FlipHorizontally;
                    this.incrementX = -32;
                    this.CollisionRay.Direction = Vector3.Left;
                    break;
                case Direction.DirectRight:
                    this.zapperPosition.X = this.holdingPlayer.CharacterPosition.X + this.holdingPlayer.DrawingOffset.X;
                    this.zapperPosition.Y = this.holdingPlayer.CharacterPosition.Y + this.holdingPlayer.DrawingOffset.Y;
                    this.currentOrigin = new Vector2(0.0f, 16.0f);
                    this.incrementX = 32;
                    this.CollisionRay.Direction = Vector3.Right;
                    break;
                case Direction.DirectUp:
                    this.zapperPosition.X = this.holdingPlayer.CharacterPosition.X + this.holdingPlayer.DrawingOffset.X;
                    this.zapperPosition.Y = this.holdingPlayer.CharacterPosition.Y + this.holdingPlayer.DrawingOffset.Y;
                    this.spriteEffects = SpriteEffects.FlipVertically;
                    this.currentOrigin = new Vector2(24.0f, 32.0f);
                    this.incrementY = -32;
                    this.CollisionRay.Direction = Vector3.Down; // This is deliberately backwards, as we're in quadarant 4 (2d space), not quadrant 1 (3d space)
                    break;
                case Direction.DirectDown:
                    this.zapperPosition.X = this.holdingPlayer.CharacterPosition.X + this.holdingPlayer.DrawingOffset.X;
                    this.zapperPosition.Y = this.holdingPlayer.CharacterPosition.Y + this.holdingPlayer.DrawingOffset.Y;
                    this.currentOrigin = new Vector2(24.0f, 0.0f);
                    this.incrementY = 32;
                    this.CollisionRay.Direction = Vector3.Up; // This is deliberately backwards, as we're in quadarant 4 (2d space), not quadrant 1 (3d space)
                    break;
                default:
                    this.CollisionRay = new Ray();
                    break;
            }

            if (this.CurrentLength > 0)
            {
                this.FindMaxLength(true);
            }
        }

        /// <summary>
        /// Draws a single player sprite
        /// </summary>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="transitionColor">The color number that should be applied to help cope with any transition effects</param>
        public void Draw(SpriteBatch mazeSpriteBatch, byte transitionColor)
        {
            Vector2 workingZapperPosition = this.zapperPosition;

            if (this.CurrentLength == 0)
            {
                return;
            }

            for (int counter = this.CurrentLength; counter > 0; counter--)
            {
                int spriteNumber = (int)this.holdingPlayer.LastDirection * 2;
                if (this.flipDirection)
                {
                    spriteNumber++;
                }

                if (counter > 1)
                {
                    spriteNumber += 8;
                }

                Rectangle spriteLocation;
                spriteLocation = spriteLocations[spriteNumber];

                mazeSpriteBatch.Draw(
                    this.miscItemsTexture, workingZapperPosition, spriteLocation, new Color(transitionColor, transitionColor, transitionColor), 0.0f, this.currentOrigin, 1.0f, this.spriteEffects, this.zapperDepth);
                workingZapperPosition.X += this.incrementX;
                workingZapperPosition.Y += this.incrementY;
            }
        }

        /// <summary>
        /// Finds the maximum length of a player's zapper given the map constraints
        /// </summary>
        /// <param name="useBeamLength">Should the player's current beam length be also used when considering the maximum</param>
        /// <returns>The length of the player's zapper in zapper tiles</returns>
        public int FindMaxLength(bool useBeamLength)
        {
            int lengthToUse;

            if (useBeamLength)
            {
                lengthToUse = this.CurrentLength;
            }
            else
            {
                lengthToUse = 4;
            }

            MazeItem testItem;
            int beamColumn = (int)Math.Round(this.holdingPlayer.CharacterPosition.X / 32);
            int beamRow = (int)Math.Round(this.holdingPlayer.CharacterPosition.Y / 32);
            int counter;
            for (counter = 0; counter < lengthToUse; counter++)
            {
                switch (this.holdingPlayer.LastDirection)
                {
                    case Direction.DirectLeft:
                        beamColumn--;
                        break;
                    case Direction.DirectRight:
                        beamColumn++;
                        break;
                    case Direction.DirectUp:
                        beamRow--;
                        break;
                    case Direction.DirectDown:
                        beamRow++;
                        break;
                }

                testItem = this.parentComponent.GetMazeItem(beamRow, beamColumn);
                if (testItem.Destructible)
                {
                    if (useBeamLength)
                    {
                        this.BeamHitItemColumn = beamColumn;
                        this.BeamHitItemRow = beamRow;
                    }

                    if (counter < 4)
                    {
                        return counter + 1;
                    }
                    else
                    {
                        return 4;
                    }
                }

                if (!testItem.IsPassable)
                {
                    return counter;
                }
            }

            return counter;
        }

        /// <summary>
        /// Converts zapper power to the number of tiles that it can travel
        /// </summary>
        /// <returns>The number of tiles long the zapper will be</returns>
        public int ZapperPowerToTiles()
        {
            if (this.ZapperPower == 0)
            {
                return 0;
            }
            else if (this.ZapperPower < 100)
            {
                return 1;
            }
            else if (this.ZapperPower < 200)
            {
                return 2;
            }
            else if (this.ZapperPower < 300)
            {
                return 3;
            }
            else if (this.ZapperPower < 400)
            {
                return 4;
            }

            return 4;
        }
    }
}
