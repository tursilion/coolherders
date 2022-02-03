//-----------------------------------------------------------------------------
// <copyright file="MovingCharacter.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Text;
    using CoolHerders.GameItems;
    using CoolHerders.Housekeeping;
    using CoolHerders.Pathfinder;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// Holds the information for, and updates and renders all types of moving characters
    /// </summary>
    internal abstract class MovingCharacter : CoolHerders.ICollidable
    {
        /// <summary>
        /// The current speed of this character
        /// </summary>
        public float CharacterSpeed = 150.0f;

        /// <summary>
        /// The offset to place on characters to make them lock to the maze grid correctly
        /// </summary>
        public Vector2 DrawingOffset = new Vector2(16, 0);

        /// <summary>
        /// The depth of the character for pseudo-3d rendering
        /// </summary>
        public float CharacterDepth;

        /// <summary>
        /// Holds the last direction the character was known to move
        /// </summary>
        public Direction LastDirection;

        /// <summary>
        /// Holds the position of the character in 2D coordinates on the screen.
        /// </summary>
        public Vector2 CharacterPosition;

        /// <summary>
        /// The character's maze location column
        /// </summary>
        public int CharacterColumn;

        /// <summary>
        /// The character's maze location row
        /// </summary>
        public int CharacterRow;

        /// <summary>
        /// Holds the texture page for this character, may or may not be a shared reference with other character
        /// </summary>
        protected Texture2D characterTexture;

        /// <summary>
        /// Is the character stopped, or are they moving in the direction lastDirection
        /// </summary>
        protected bool characterStopped;

        /// <summary>
        /// Is the character frozen (don't snap to grid)
        /// </summary>
        protected bool characterFrozen;

        /// <summary>
        /// Can this type of character destroy boxes
        /// </summary>
        protected bool canDestroyBoxes = false;

        /// <summary>
        /// Holds the position of the character's texture on the texture page
        /// </summary>
        protected Rectangle characterTextureLocation;

        /// <summary>
        /// Holds the Maze Game Component into which we are going to be drawn
        /// </summary>
        protected MazeGameComponent parentComponent;

        /// <summary>
        /// Is it okay to move west?
        /// </summary>
        protected bool westOk = true;

        /// <summary>
        /// Is it okay to move east?
        /// </summary>
        protected bool eastOk = true;

        /// <summary>
        /// Is it okay to move north?
        /// </summary>
        protected bool northOk = true;

        /// <summary>
        /// Is it okay to move south?
        /// </summary>
        protected bool southOk = true;

        /// <summary>
        /// Can the tile to the north be destroyed
        /// </summary>
        protected bool northDestructable = false;

        /// <summary>
        /// Can the tile to the south be destroyed
        /// </summary>
        protected bool southDestructable = false;

        /// <summary>
        /// Can the tile to the east be destroyed
        /// </summary>
        protected bool eastDestructable = false;

        /// <summary>
        /// Can the tile to the west be destroyed
        /// </summary>
        protected bool westDestructable = false;

        /// <summary>
        /// Is the character being shoved backwards
        /// </summary>
        protected bool beingShoved = false;

        /// <summary>
        /// A wave bank for our effects
        /// </summary>
        [SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification = "We must load this, but XACT never uses this reference")]
        protected WaveBank characterVoiceWaveBank;

        /// <summary>
        /// The sound bank for this character voice
        /// </summary>
        protected SoundBank characterVoiceSoundBank;

        /// <summary>
        /// Can we zap this sprite
        /// </summary>
        protected bool checkZapperCollisions = true;

        /// <summary>
        /// Information about this player, which may well be an AI
        /// </summary>
        protected PlayerInformation playerInformation;

        /// <summary>
        /// The number of steps in the character's animation
        /// </summary>
        private const int AnimationSteps = 4;

        /// <summary>
        /// The bounding box that will be used for the collision
        /// </summary>
        private BoundingBox collisionBox;

        /// <summary>
        /// Can we collide with this sprite
        /// </summary>
        private bool checkCollisions = true;

        /// <summary>
        /// This tracks the time the animation has been running
        /// </summary>
        private TimeSpan animationTime = TimeSpan.Zero;

        /// <summary>
        /// The state of the background animation
        /// </summary>
        private int animationState;

        /// <summary>
        /// The X coordinate at which we will reevaluate the character's position in the maze if it is moving left
        /// </summary>
        private double tripwireLeft;

        /// <summary>
        /// The X coordinate at which we will reevaluate the character's position in the maze if it is moving right
        /// </summary>
        private double tripwireRight;

        /// <summary>
        /// The Y coordinate at which we will reevaluate the character's position in the maze if it is moving up
        /// </summary>
        private double tripwireUp;

        /// <summary>
        /// The Y coordinate at which we will reevaluate the character's position in the maze if it is moving down
        /// </summary>
        private double tripwireDown;

        /// <summary>
        /// The initial position of this character in screen coordinates
        /// </summary>
        private Vector2 initialPositon;

        /// <summary>
        /// The network gamer which is controlling this character
        /// </summary>
        protected NetworkGamer networkGamer;

        /// <summary>
        /// Initializes a new instance of the MovingCharacter class.
        /// </summary>
        /// <param name="parentComponent">The game component that is managing this moving character</param>
        /// <param name="playerInformation">The player information about this player</param>
        /// <param name="rowCounter">The row to start the player at</param>
        /// <param name="columnCounter">The column to start the player at</param>
        protected MovingCharacter(MazeGameComponent parentComponent, NetworkGamer networkGamer, PlayerInformation playerInformation, int rowCounter, int columnCounter)
        {
            if ((rowCounter < 0) || (rowCounter > 16))
            {
                throw new ArgumentOutOfRangeException("rowCounter", "Row counter must be between 0 and 15");
            }

            if ((columnCounter < 0) || (columnCounter > 22))
            {
                throw new ArgumentOutOfRangeException("columnCounter", "Column counter must be between 0 and 21");
            }

            if (networkGamer == null)
            {
                throw new ArgumentNullException("networkGamer", "A valid network gamer must be supplied.");
            }

            this.parentComponent = parentComponent;
            this.playerInformation = playerInformation;
            this.networkGamer = networkGamer;

            this.CharacterPosition = new Vector2(columnCounter * 32, rowCounter * 32);
            this.initialPositon = this.CharacterPosition;
            this.CharacterRow = (int)Math.Round(this.CharacterPosition.Y / 32);
            this.CharacterColumn = (int)Math.Round(this.CharacterPosition.X / 32);

            this.characterStopped = true;
            this.SetTripwires();

            string playerColor;
            if (playerInformation.PlayerColorIndex != 255)
            {
                if (playerInformation.PlayerColorIndex < 16)
                {
                    playerColor = string.Format("{0:X}", playerInformation.PlayerColorIndex);
                }
                else
                {
                    playerColor = playerInformation.PlayerColorIndex.ToString(CultureInfo.InvariantCulture);
                }
            } else {
                playerColor = "WH";
            }
            for (; ; )
            {
                try
                {
                    this.characterTexture = parentComponent.PlayGameContent.Load<Texture2D>(string.Format(CultureInfo.CurrentCulture, "Characters\\{0}\\{0}{1}", playerInformation.CharacterClass, playerColor));
                }
                catch (Microsoft.Xna.Framework.Content.ContentLoadException e)
                {
                    Console.WriteLine("Load exception " + e.ToString());
                    playerInformation.PlayerColorIndex++;
                    if (playerInformation.PlayerColorIndex > 15)
                    {
                        playerInformation.PlayerColorIndex = 0;
                    }
                    playerColor = string.Format("{0:X}", playerInformation.PlayerColorIndex);
                    continue;
                }
                break;
            }
            this.characterTextureLocation = new Rectangle(0, 0, 48, 48);

            this.parentComponent.GameScreen.GameHasFinished += new EventHandler<EventArgs>(this.GameScreen_GameHasFinished);
        }

        /// <summary>
        /// Gets or sets the collision box
        /// </summary>
        public BoundingBox CollisionBox
        {
            get { return this.collisionBox; }
            set { this.collisionBox = value; }
        }

        /// <summary>
        /// Gets or sets a value indicating whether collisions should be checked
        /// </summary>
        public bool CheckCollisions
        {
            get { return this.checkCollisions; }
            set { this.checkCollisions = value; }
        }

        /// <summary>
        /// Draws a single player sprite
        /// </summary>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="transitionColor">The color number that should be applied to help cope with any transition effects</param>
        /// <param name="alphaChannel">An additional alpha channel to use when drawing the character</param>
        public virtual void Draw(SpriteBatch mazeSpriteBatch, byte transitionColor, byte alphaChannel)
        {
            mazeSpriteBatch.Draw(
                this.characterTexture, this.CharacterPosition + this.DrawingOffset, this.characterTextureLocation, new Color(transitionColor, transitionColor, transitionColor, alphaChannel), 0.0f, new Vector2(24.0f, 24.0f), 1.0f, SpriteEffects.None, this.CharacterDepth);
        }

        /// <summary>
        /// Checks a collision given an array of characacters
        /// </summary>
        /// <param name="otherCharacters">An array of characters</param>
        /// <returns>True if one or more of the characters is colliding with THIS character</returns>
        public bool CheckCollision(ICollidable[] otherCharacters)
        {
            bool returnValue = false;

            foreach (MovingCharacter otherCharacter in otherCharacters)
            {
                if ((otherCharacter != this) && (otherCharacter != null))
                {
                    returnValue |= this.CheckCollision(otherCharacter);
                }
            }

            return returnValue;
        }

        /// <summary>
        /// Checks for a collision between this character and some other character
        /// </summary>
        /// <param name="otherCharacter">The other character</param>
        /// <returns>True if the other character overlaps us</returns>
        public bool CheckCollision(ICollidable otherCharacter)
        {
            if (this.checkCollisions && otherCharacter.CheckCollisions)
            {
                return this.CollisionBox.Intersects(otherCharacter.CollisionBox);
            }

            return false;
        }

        /// <summary>
        /// Given a specific bounding box, check collision with that item
        /// </summary>
        /// <param name="collisionBox">The other collision box</param>
        /// <returns>True if the collision box overlaps this character's</returns>
        public bool CheckCollision(BoundingBox collisionBox)
        {
            if (this.checkCollisions)
            {
                return this.CollisionBox.Intersects(collisionBox);
            }

            return false;
        }

        /// <summary>
        /// Given a direction, does a coarse check to see if this moving character is somewhere along the infinite length ray
        /// </summary>
        /// <param name="collisionRay">The ray to check</param>
        /// <returns>NULL if no collision occured, or the distance at which the collision occured</returns>
        public float? CheckCollision(Ray collisionRay)
        {
            if (this.checkCollisions && this.checkZapperCollisions)
            {
                return this.CollisionBox.Intersects(collisionRay);
            }

            return null;
        }

        /// <summary>
        /// Updates the player character
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="obstructingObjects">Any objects which we should not pass through</param>
        /// <param name="collectableObjects">Any objects which we may collect</param>
        internal virtual void Update(GameTime gameTime, ICollidable[] obstructingObjects, ICollidable[] collectableObjects)
        {
            this.animationTime -= gameTime.ElapsedGameTime;
            if (this.animationTime.Seconds <= 0)
            {
                this.animationState = (this.animationState + 1) % 4;
                this.animationTime = this.animationTime.Add(TimeSpan.FromMilliseconds(250));
            }

            this.CharacterRow = (int)Math.Round(this.CharacterPosition.Y / 32);
            this.CharacterColumn = (int)Math.Round(this.CharacterPosition.X / 32);

            PlayerIndex playerIndex = GameInformation.Instance.MasterPlayerIndex;
            if (this.playerInformation.SignedInGamer != null)
            {
                playerIndex = this.playerInformation.SignedInGamer.PlayerIndex;
            }
            bool atMazeJunction = this.CheckExitsAtMazeJunction();
            this.HandleMovementChange(playerIndex, atMazeJunction, collectableObjects);

            this.SetViewFromDirection(this.characterStopped);

            this.PerformCharacterMovement(gameTime);

            this.CharacterDepth = ((this.CharacterRow - 1) * 0.05f) + ((this.CharacterColumn + 1) * 0.002f) + 0.009f;
            this.CollisionBox = new BoundingBox(new Vector3(this.CharacterPosition.X, this.CharacterPosition.Y, 0.0f), new Vector3(this.CharacterPosition.X + 24, this.CharacterPosition.Y + 16, 0.0f));
        }

        /// <summary>
        /// Called when the character needs to update it's movement
        /// </summary>
        /// <param name="playerIndex">The player index</param>
        /// <param name="atMazeJunction">Is this character at a maze junction right now</param>
        /// <param name="collectableObjects">Objects within the maze, such as sheep or pickups, which may be collected by this character</param>
        protected abstract void HandleMovementChange(PlayerIndex playerIndex, bool atMazeJunction, ICollidable[] collectableObjects);

        /// <summary>
        /// Selects the correct view given the direction the player is facing
        /// </summary>
        /// <param name="playerStopped">True if the player is stopped</param>
        protected void SetViewFromDirection(bool playerStopped)
        {
            if (playerStopped)
            {
                this.characterTextureLocation = new Rectangle(((int)this.LastDirection * 48), 4 * 48, 48, 48);
            }
            else
            {
                this.characterTextureLocation = new Rectangle(((int)this.LastDirection * 48), this.animationState * 48, 48, 48);
            }
        }

        /// <summary>
        /// A quick-and-dirty check to see how many exits there are from the current junction
        /// </summary>
        /// <returns>Number of directions</returns>
        protected int GetNumberOfExits()
        {
            int number = 0;
            if (this.northOk)
            {
                number++;
            }

            if (this.southOk)
            {
                number++;
            }

            if (this.eastOk)
            {
                number++;
            }

            if (this.westOk)
            {
                number++;
            }

            return number;
        }

        /// <summary>
        /// Turns the moving character clockwise, based on the legality of the maze
        /// </summary>
        /// <returns>True if the character could be turned that direction</returns>
        protected bool TurnCharacterClockwise()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.westOk)
                    {
                        this.LastDirection = Direction.DirectLeft;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.northOk)
                    {
                        this.LastDirection = Direction.DirectUp;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.eastOk)
                    {
                        this.LastDirection = Direction.DirectRight;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.southOk)
                    {
                        this.LastDirection = Direction.DirectDown;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Turns a character counter-clockwise, based on the legality of the maze
        /// </summary>
        /// <returns>True if the character was so turned</returns>
        protected bool TurnCharacterCounterClockwise()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.eastOk)
                    {
                        this.LastDirection = Direction.DirectRight;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.southOk)
                    {
                        this.LastDirection = Direction.DirectDown;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.westOk)
                    {
                        this.LastDirection = Direction.DirectLeft;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.northOk)
                    {
                        this.LastDirection = Direction.DirectUp;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Turns the character backwards, based on the legality of the maze
        /// </summary>
        /// <returns>True if the character was so turned</returns>
        protected bool TurnCharacterReverse()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.northOk)
                    {
                        this.LastDirection = Direction.DirectUp;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.eastOk)
                    {
                        this.LastDirection = Direction.DirectRight;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.southOk)
                    {
                        this.LastDirection = Direction.DirectDown;
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.westOk)
                    {
                        this.LastDirection = Direction.DirectLeft;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Checks to see if the character can go backwards
        /// </summary>
        /// <returns>True if the maze allows movement in that direction</returns>
        protected bool CanGoBackwards()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.northOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.eastOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.southOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.westOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Checks to see if the character can go forwards
        /// </summary>
        /// <returns>True if the character can go that direction</returns>
        protected bool CanGoForward()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.southOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.westOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.northOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.eastOk)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Checks to see if the character can destroy something forwards
        /// </summary>
        /// <returns>True if the character can destroy something in that direction</returns>
        protected bool CanDestroyForward()
        {
            switch (this.LastDirection)
            {
                case Direction.DirectDown:
                    if (this.southDestructable)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectLeft:
                    if (this.westDestructable)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectUp:
                    if (this.northDestructable)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }

                case Direction.DirectRight:
                    if (this.eastDestructable)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
            }

            return false;
        }

        /// <summary>
        /// Gets the direction we should face in order to look at a given player
        /// </summary>
        /// <param name="attackingPlayer">The player that is attacking this one</param>
        protected void GetDirectionFromGridDifference(PlayerCharacter attackingPlayer)
        {
            if ((attackingPlayer.CharacterColumn == this.CharacterColumn) && (attackingPlayer.CharacterRow < this.CharacterRow))
            {
                this.LastDirection = Direction.DirectUp;
                this.CharacterPosition.X = (float)Math.Round(this.CharacterPosition.X / 32.0f) * 32.0f;
            }
            else if ((attackingPlayer.CharacterColumn == this.CharacterColumn) && (attackingPlayer.CharacterRow > this.CharacterRow))
            {
                this.LastDirection = Direction.DirectDown;
                this.CharacterPosition.X = (float)Math.Round(this.CharacterPosition.X / 32.0f) * 32.0f;
            }
            else if ((attackingPlayer.CharacterColumn < this.CharacterColumn) && (attackingPlayer.CharacterRow == this.CharacterRow))
            {
                this.LastDirection = Direction.DirectLeft;
                this.CharacterPosition.Y = (float)Math.Round(this.CharacterPosition.Y / 32.0f) * 32.0f;
            }
            else if ((attackingPlayer.CharacterColumn > this.CharacterColumn) && (attackingPlayer.CharacterRow == this.CharacterRow))
            {
                this.LastDirection = Direction.DirectRight;
                this.CharacterPosition.Y = (float)Math.Round(this.CharacterPosition.Y / 32.0f) * 32.0f;
            }
        }

        /// <summary>
        /// Gets the direction this herder must face in order to face a given destination
        /// </summary>
        /// <param name="cellColumn">Current cell column</param>
        /// <param name="cellRow">Current cell row</param>
        /// <param name="destinationCoordinate">Destination coordinate from the router</param>
        protected void GetDirectionFromGridDifference(int cellColumn, int cellRow, MazeCoordinate destinationCoordinate)
        {
            this.GetDirectionFromGridDifference(cellColumn, cellRow, destinationCoordinate.CellColumn, destinationCoordinate.CellRow);
        }

        /// <summary>
        /// Gets the direction this herder must face in order to face a given destination
        /// </summary>
        /// <param name="cellColumn">Current cell column</param>
        /// <param name="cellRow">Current cell row</param>
        /// <param name="destinationCellColumn">Destination cell column</param>
        /// <param name="destinationCellRow">Destination cell row</param>
        protected void GetDirectionFromGridDifference(int cellColumn, int cellRow, int destinationCellColumn, int destinationCellRow)
        {
            if ((destinationCellColumn == cellColumn) && (destinationCellRow < cellRow))
            {
                this.LastDirection = Direction.DirectUp;
            }
            else if ((destinationCellColumn == cellColumn) && (destinationCellRow > cellRow))
            {
                this.LastDirection = Direction.DirectDown;
            }
            else if ((destinationCellColumn < cellColumn) && (destinationCellRow == cellRow))
            {
                this.LastDirection = Direction.DirectLeft;
            }
            else if ((destinationCellColumn > cellColumn) && (destinationCellRow == cellRow))
            {
                this.LastDirection = Direction.DirectRight;
            }
        }

        /// <summary>
        /// Performs the actual character movement
        /// </summary>
        /// <param name="gameTime">The current game time</param>
        private void PerformCharacterMovement(GameTime gameTime)
        {
            float distanceMoved;

            distanceMoved = (float)gameTime.ElapsedGameTime.TotalSeconds * this.CharacterSpeed;

            if (!this.characterStopped)
            {
                switch (this.LastDirection)
                {
                    case Direction.DirectUp:
                        this.CharacterPosition.Y -= distanceMoved;
                        this.northOk = true;
                        this.southOk = true;
                        this.eastOk = false;
                        this.westOk = false;
                        break;
                    case Direction.DirectDown:
                        this.CharacterPosition.Y += distanceMoved;
                        this.northOk = true;
                        this.southOk = true;
                        this.eastOk = false;
                        this.westOk = false;
                        break;
                    case Direction.DirectLeft:
                        this.CharacterPosition.X -= distanceMoved;
                        this.northOk = false;
                        this.southOk = false;
                        this.eastOk = true;
                        this.westOk = true;
                        break;
                    case Direction.DirectRight:
                        this.CharacterPosition.X += distanceMoved;
                        this.northOk = false;
                        this.southOk = false;
                        this.eastOk = true;
                        this.westOk = true;
                        break;
                }
            }
        }

        /// <summary>
        /// Resets the tripwires that the character will use to see if it's in a new cell
        /// </summary>
        protected void SetTripwires()
        {
            this.tripwireUp = Math.Floor(this.CharacterPosition.Y / 32.0f) * 32.0f;
            this.tripwireDown = Math.Ceiling(this.CharacterPosition.Y / 32.0f) * 32.0f;
            this.tripwireLeft = Math.Floor(this.CharacterPosition.X / 32.0f) * 32.0f;
            this.tripwireRight = Math.Ceiling(this.CharacterPosition.X / 32.0f) * 32.0f;
        }

        /// <summary>
        /// Checks if we are at a maze junction, and rereads the directional information from the main maze object
        /// </summary>
        /// <returns>True if we are at a maze junction</returns>
        protected bool CheckExitsAtMazeJunction()
        {
            bool atMazeJunction = false;

            if (((this.tripwireLeft != this.tripwireRight) && ((this.CharacterPosition.X < this.tripwireLeft) || (this.CharacterPosition.X > this.tripwireRight))) ||
                ((this.tripwireUp != this.tripwireDown) && ((this.CharacterPosition.Y < this.tripwireUp) || (this.CharacterPosition.Y > this.tripwireDown))) ||
                (((this.CharacterPosition.X % 32) == 0) && ((this.CharacterPosition.Y % 32) == 0)))
            {
                try
                {
                    this.eastOk = this.parentComponent.GetMazeItem(CharacterRow, CharacterColumn + 1).IsPassable;
                    this.westOk = this.parentComponent.GetMazeItem(CharacterRow, CharacterColumn - 1).IsPassable;
                    this.northOk = this.parentComponent.GetMazeItem(CharacterRow - 1, CharacterColumn).IsPassable;
                    this.southOk = this.parentComponent.GetMazeItem(CharacterRow + 1, CharacterColumn).IsPassable;
                }
                catch (ArgumentOutOfRangeException)
                {
                    // If we've gone out of range, reset us to our initial position
                    // It's a hack, but so is having such a long delay that we leave the map
                    this.CharacterPosition = this.initialPositon;
                    this.CharacterRow = (int)Math.Round(this.CharacterPosition.Y / 32);
                    this.CharacterColumn = (int)Math.Round(this.CharacterPosition.X / 32);
                    this.SetTripwires();
                }

                if (this.canDestroyBoxes)
                {
                    this.eastDestructable = this.parentComponent.GetMazeItem(CharacterRow, CharacterColumn + 1).Destructible;
                    this.westDestructable = this.parentComponent.GetMazeItem(CharacterRow, CharacterColumn - 1).Destructible;
                    this.northDestructable = this.parentComponent.GetMazeItem(CharacterRow - 1, CharacterColumn).Destructible;
                    this.southDestructable = this.parentComponent.GetMazeItem(CharacterRow + 1, CharacterColumn).Destructible;
                }
                else
                {
                    this.eastDestructable = false;
                    this.westDestructable = false;
                    this.northDestructable = false;
                    this.southDestructable = false;
                }

                if (!this.beingShoved)
                {
                    // If we're not being shoved, then stop at a junction
                    this.characterStopped = true;
                }
                else
                {
                    this.characterStopped = !this.CanGoBackwards();
                }
                
                this.CharacterPosition = new Vector2(this.CharacterColumn * 32, this.CharacterRow * 32);
                atMazeJunction = true;
            }

            this.SetTripwires();

            return atMazeJunction;
        }

        /// <summary>
        /// Callback for when the game has finished
        /// </summary>
        /// <param name="sender">The object that sent his message</param>
        /// <param name="e">Standard event arguments</param>
        private void GameScreen_GameHasFinished(object sender, EventArgs e)
        {
            this.checkZapperCollisions = false;
        }
    }
}
