//-----------------------------------------------------------------------------
// <copyright file="PlayerCharacter.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using CoolHerders.Housekeeping;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// This tracks the player character as it moves around the maze
    /// </summary>
    internal class PlayerCharacter : MovingCharacter
    {
        /// <summary>
        /// The default speed for a player
        /// </summary>
        const float DefaultPlayerSpeed = 175.0f;

        /// <summary>
        /// The player's zapper gun
        /// </summary>
        public ElectricZapper PlayerZapper;

        /// <summary>
        /// The number of samples of player position and direction we should keep in a history
        /// </summary>
        protected const int NumberOfSamples = 200;

        /// <summary>
        /// The amount of time between position samples
        /// </summary>
        protected static readonly TimeSpan PositionTimeout = TimeSpan.FromMilliseconds(10);

        /// <summary>
        /// Holds the controller input for this player
        /// </summary>
        protected InputState playerInputState;

        /// <summary>
        /// An array holding the previous positions we have been at
        /// </summary>
        protected Vector2[] previousPositions;

        /// <summary>
        /// An array holding the previous directions we have faced
        /// </summary>
        protected Direction[] previousDirections;

        /// <summary>
        /// The head of the previous position ring buffer
        /// </summary>
        protected int positionHead;

        /// <summary>
        /// The tail of the previous position ring buffer
        /// </summary>
        protected int positionTail;

        /// <summary>
        /// The time till we sample the next position
        /// </summary>
        protected TimeSpan positionTimer;

        /// <summary>
        /// How much longer should the player remain invincible for
        /// </summary>
        protected TimeSpan invincibleTime = TimeSpan.Zero;

        /// <summary>
        /// How much longer should the player's controller vibrate for
        /// </summary>
        protected TimeSpan vibrateTime = TimeSpan.Zero;

        /// <summary>
        /// How much longer should the player get shoved
        /// </summary>
        protected TimeSpan shoveTime = TimeSpan.Zero;

        /// <summary>
        /// How long should a player remain invincible for
        /// </summary>
        private static readonly TimeSpan invincibleDuration = TimeSpan.FromMilliseconds(800);

        /// <summary>
        /// How long should a player's controller vibrate for
        /// </summary>
        private static readonly TimeSpan vibrationDuration = TimeSpan.FromMilliseconds(250);

        /// <summary>
        /// How long should a player be shoved for
        /// </summary>
        private static readonly TimeSpan shoveDuration = TimeSpan.FromMilliseconds(500);

        /// <summary>
        /// The number of sheep this player is toting
        /// </summary>
        private int numberOfSheeps;

        /// <summary>
        /// The index within the history arrays the sheep should use for following
        /// </summary>
        private int sheepPositionIndexer;

        /// <summary>
        /// The sheep that are following this player
        /// </summary>
        private List<SheepCharacter> followingSheepies;

        public float SpeedPowerupAmount = 0.0f;

        protected int destinationColumn;

        protected int destinationRow;

        /// <summary>
        /// Initializes a new instance of the PlayerCharacter class.
        /// </summary>
        /// <param name="parentComponent">The parent component on which this class will be drawn</param>
        /// <param name="gamerClass">The class holding the gamer information, or the owning player info if a computer managed creation</param>
        /// <param name="playerClassName">The type of player we are creating, by name</param>
        /// <param name="subclass">The subclass name for this player</param>
        /// <param name="rowCounter">The starting row of this player</param>
        /// <param name="columnCounter">The starting column of this player</param>
        public PlayerCharacter(MazeGameComponent parentComponent, NetworkGamer networkGamer, PlayerInformation playerInformation, int rowCounter, int columnCounter)
            : base(parentComponent, networkGamer, playerInformation, rowCounter, columnCounter)
        {
            this.playerInputState = this.parentComponent.MazeInputState;

            this.PlayerZapper = new ElectricZapper(parentComponent, this);
            this.previousPositions = new Vector2[NumberOfSamples];
            this.previousDirections = new Direction[NumberOfSamples];
            for (int counter = 0; counter < NumberOfSamples; counter++)
            {
                this.previousPositions[counter] = this.CharacterPosition;
                this.previousDirections[counter] = this.LastDirection;
            }

            this.positionTail = NumberOfSamples - 1;
            this.followingSheepies = new List<SheepCharacter>(50);

            this.CharacterRow = (int)Math.Round(this.CharacterPosition.Y / 32);
            this.CharacterColumn = (int)Math.Round(this.CharacterPosition.X / 32);
            GetDirectionFromGridDifference(this.CharacterColumn, this.CharacterRow, 8, this.CharacterRow);

            AudioEngine ourAudioEngine = ((CoolHerdersGame)this.parentComponent.GameScreen.ScreenManager.Game).GameAudioEngine;
            this.characterVoiceWaveBank = new WaveBank(ourAudioEngine, string.Format("Content\\Audio\\{0}Voices.xwb", playerInformation.CharacterClass));
            this.characterVoiceSoundBank = new SoundBank(ourAudioEngine, string.Format("Content\\Audio\\{0}Voice.xsb", playerInformation.CharacterClass));
        }

        /// <summary>
        /// Gets a value indicating the number of sheep this player holds
        /// </summary>
        public int NumberOfSheep
        {
            get
            {
                return this.numberOfSheeps;
            }
        }

        /// <summary>
        /// Draws a single player sprite
        /// </summary>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="transitionColor">The color number that should be applied to help cope with any transition effects</param>
        /// <param name="alphaChannel">An additional alpha channel to use when drawing the character</param>
        public override void Draw(SpriteBatch mazeSpriteBatch, byte transitionColor, byte alphaChannel)
        {
            if (this.invincibleTime.Ticks > 0)
            {
                base.Draw(mazeSpriteBatch, transitionColor, 150);
            }
            else
            {
                base.Draw(mazeSpriteBatch, transitionColor, alphaChannel);
            }

            // Draw our child objects
            this.PlayerZapper.Draw(mazeSpriteBatch, transitionColor);
        }

        /// <summary>
        /// Gets the player's position at a given moment in time
        /// </summary>
        /// <param name="positionNumber">The steps prior to this moment in time</param>
        /// <returns>The player's position at the given moment in time</returns>
        public Vector2 GetPreviousPosition(int positionNumber)
        {
            int tempPosition = this.positionHead - positionNumber;
            if (tempPosition < 0)
            {
                tempPosition = tempPosition + NumberOfSamples;
            }

            return this.previousPositions[tempPosition];
        }

        /// <summary>
        /// Gets the player's direction at a given moment in time
        /// </summary>
        /// <param name="positionNumber">The steps prior to this moment in time</param>
        /// <returns>The player's position at the given moment in time</returns>
        public Direction GetPreviousDirection(int positionNumber)
        {
            int tempPosition = this.positionHead - positionNumber;
            if (tempPosition < 0)
            {
                tempPosition = tempPosition + NumberOfSamples;
            }

            return this.previousDirections[tempPosition];
        }

        /// <summary>
        /// Adds a sheep to the player's list of sheep
        /// </summary>
        /// <param name="theSheep">The sheep object that is now following the player</param>
        /// <returns>The number of steps back that the sheep should follow</returns>
        public int AddASheep(SheepCharacter theSheep)
        {
            this.numberOfSheeps++;
            this.sheepPositionIndexer += 3;
            this.followingSheepies.Add(theSheep);
            return this.sheepPositionIndexer;
        }

        /// <summary>
        /// Checks a collision between our zapper and some other character
        /// </summary>
        /// <param name="movingCharacter">The character to see if we have collided with.</param>
        /// <returns>True if a collision occurred</returns>
        public bool CheckZapperCollision(MovingCharacter movingCharacter)
        {
            if (this.PlayerZapper.CurrentLength > 0)
            {
                float? characterDistance = movingCharacter.CheckCollision(this.PlayerZapper.CollisionRay);
                if (null != characterDistance)
                {
                    if ((this.PlayerZapper.CurrentLength * 32) >= characterDistance)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Drops a sheep if the player is carrying one
        /// </summary>
        /// <param name="attackingPlayer">The player that performed the attack</param>
        /// <returns>True if a sheep was dropped</returns>
        public virtual bool DropSheep(PlayerCharacter attackingPlayer)
        {
            if (this.playerInformation.SignedInGamer != null)
            {
                GamePad.SetVibration(this.playerInformation.SignedInGamer.PlayerIndex, 0.0f, 0.5f);
            }

            this.characterVoiceSoundBank.PlayCue("Damage");
            this.vibrateTime = vibrationDuration;
            this.invincibleTime = invincibleDuration;
            this.shoveTime = shoveDuration;
            this.checkZapperCollisions = false;
            this.beingShoved = true;
            this.characterStopped = false;
            this.CharacterSpeed = -300.0f;
            this.SpeedPowerupAmount = 0.0f;
            this.PlayerZapper.StopFiring();

            GetDirectionFromGridDifference(attackingPlayer);

            if (this.numberOfSheeps > 0)
            {
                this.followingSheepies[this.followingSheepies.Count - 1].RunFree();
                this.followingSheepies.RemoveAt(this.followingSheepies.Count - 1);
                this.numberOfSheeps--;
                this.sheepPositionIndexer -= 3;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Updates the player character
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="obstructingObjects">Objects which we may not move through</param>
        /// <param name="collectableObjects">Objects which we may collect</param>
        internal override void Update(GameTime gameTime, ICollidable[] obstructingObjects, ICollidable[] collectableObjects)
        {
            Vector2 previousCharPosition = this.CharacterPosition;

            base.Update(gameTime, obstructingObjects, collectableObjects);

            if (CheckCollision(obstructingObjects))
            {
                this.CharacterPosition = previousCharPosition;
                this.CollisionBox = new BoundingBox(new Vector3(this.CharacterPosition.X, this.CharacterPosition.Y, 0.0f), new Vector3(this.CharacterPosition.X + 24, this.CharacterPosition.Y + 16, 0.0f));
                this.HandleObstructingCollision();
            }

            foreach (PlayerCharacter player in obstructingObjects)
            {
                if (null != player)
                {
                    if (player != this)
                    {
                        if (this.CheckZapperCollision(player))
                        {
                            player.DropSheep(this);
                            this.HandleZappedSomething();
                        }
                    }
                }
            }

            if (null != collectableObjects)
            {
                foreach (SheepCharacter sheepie in collectableObjects)
                {
                    if (null != sheepie)
                    {
                        if (CheckCollision(sheepie))
                        {
                            this.HandleSheepTouch(sheepie);
                        }

                        if (this.CheckZapperCollision(sheepie))
                        {
                            sheepie.Stun();
                            this.HandleZappedSheep(sheepie.CharacterColumn, sheepie.CharacterRow);
                        }
                    }
                }
            }

            PickupItem pickupClaimed = null;
            foreach (PickupItem pickupItem in this.parentComponent.PickupItems)
            {
                if (CheckCollision(pickupItem))
                {
                    pickupClaimed = pickupItem;
                    pickupItem.OnPickup(this);
                }
            }

            if (pickupClaimed != null)
            {
                this.parentComponent.PickupItems.Remove(pickupClaimed);
            }

            // Update the player zapper, have to handle our child objects
            this.PlayerZapper.Update(gameTime);

            if (this.PlayerZapper.CurrentLength > 0)
            {
                MazeItem mazeItem = this.parentComponent.GetMazeItem(this.PlayerZapper.BeamHitItemRow, this.PlayerZapper.BeamHitItemColumn);
                if (mazeItem.Destructible)
                {
                    ((DestructableMazeItem)mazeItem).StartDestruction();
                }
            }

            this.positionTimer -= gameTime.ElapsedGameTime;
            if (this.positionTimer.TotalMilliseconds <= 0)
            {
                this.previousPositions[this.positionHead] = this.CharacterPosition;
                this.previousDirections[this.positionHead] = this.LastDirection;
                this.positionHead = (this.positionHead + 1) % NumberOfSamples;
                this.positionTail = (this.positionTail + 1) % NumberOfSamples;
                this.positionTimer = this.positionTimer.Add(PositionTimeout);
            }

            if (this.invincibleTime.Ticks > 0)
            {
                this.invincibleTime -= gameTime.ElapsedGameTime;
                if (this.invincibleTime.Ticks <= 0)
                {
                    this.checkZapperCollisions = true;
                }
            }

            if (this.shoveTime.Ticks > 0)
            {
                this.shoveTime -= gameTime.ElapsedGameTime;
                if (this.shoveTime.Ticks <= 0)
                {
                    this.FinishBeingShoved();
                }
            }

            if (this.vibrateTime.Ticks > 0)
            {
                this.vibrateTime -= gameTime.ElapsedGameTime;
                if (this.vibrateTime.Ticks <= 0)
                {
                    if (this.playerInformation.SignedInGamer != null)
                    {
                        GamePad.SetVibration(this.playerInformation.SignedInGamer.PlayerIndex, 0.0f, 0.0f);
                    }
                }
            }
        }

        /// <summary>
        /// Called when this player has zapped something
        /// </summary>
        protected virtual void HandleZappedSomething()
        {
            return;
        }

        /// <summary>
        /// Called when this player has zapped a sheep
        /// </summary>
        /// <param name="sheepColumn">Cell column of the sheep</param>
        /// <param name="sheepRow">Cell row of the sheep</param>
        protected virtual void HandleZappedSheep(int sheepColumn, int sheepRow)
        {
            return;
        }

        /// <summary>
        /// Called when the shove routine is done
        /// </summary>
        protected virtual void FinishBeingShoved()
        {
            this.beingShoved = false;
            this.characterStopped = true;
            SetCharacterSpeed();
        }

        /// <summary>
        /// Sets the character speed based on all parameters
        /// </summary>
        public virtual void SetCharacterSpeed()
        {
            this.CharacterSpeed = DefaultPlayerSpeed - (this.numberOfSheeps * 4.0f) + this.SpeedPowerupAmount;
            if (this.CharacterSpeed < 100.0f)
            {
                this.CharacterSpeed = 100.0f;
            }
        }

        /// <summary>
        /// Called when this player has touched a sheep
        /// </summary>
        /// <param name="sheepie">The sheep object that was touched</param>
        protected virtual void HandleSheepTouch(SheepCharacter sheepie)
        {
            SetCharacterSpeed();
            sheepie.LockToPlayer(this);
        }

        /// <summary>
        /// Called when the player runs in to something it cannot pick up
        /// </summary>
        protected virtual void HandleObstructingCollision()
        {
            this.characterStopped = true;
        }

        /// <summary>
        /// Checks the gamepad and the options for moving, and sets the movement flags accordingly
        /// </summary>
        /// <param name="playerIndex">The player index we are playing as</param>
        /// <param name="atMazeJunction">Is this player at a maze junction right now</param>
        /// <param name="collectableObjects">Objects within the maze which may be picked up by this player</param>
        protected override void HandleMovementChange(PlayerIndex playerIndex, bool atMazeJunction, ICollidable[] collectableObjects)
        {
            // keyboard override
            bool fire = false;

            if (this.beingShoved)
            {
                return;
            }

            GamePadState ourInput = this.playerInputState.CurrentGamePadStates[(int)playerIndex];
            if ((int)playerIndex == 0)
            {
                // TODO: Remove this keyboard hack in the end.
                bool pressed = false;
                ButtonState down = ourInput.DPad.Down;
                ButtonState up = ourInput.DPad.Up;
                ButtonState left = ourInput.DPad.Left;
                ButtonState right = ourInput.DPad.Right;

                KeyboardState key = Keyboard.GetState();
                if (key.IsKeyDown(Keys.Left)) { left = ButtonState.Pressed; pressed = true; }
                if (key.IsKeyDown(Keys.Right)) { right = ButtonState.Pressed; pressed = true; }
                if (key.IsKeyDown(Keys.Up)) { up = ButtonState.Pressed; pressed = true; }
                if (key.IsKeyDown(Keys.Down)) { down = ButtonState.Pressed; pressed = true; }
                if (key.IsKeyDown(Keys.Space)) { fire = true; pressed = true; }

                if (pressed == true)
                {
                    ourInput = new GamePadState(ourInput.ThumbSticks, ourInput.Triggers, new GamePadButtons(fire==true ? Buttons.A : 0), new GamePadDPad(up, down, left, right));
                }
            }

            if ((this.playerInputState.IsNewButtonPress(Buttons.A) || fire==true) && this.checkZapperCollisions)
            {
                this.PlayerZapper.FireZapper();
            }

            if ((LastDirection == Direction.DirectUp) || (LastDirection == Direction.DirectDown))
            {
                if (!this.HandleXJoystick(ourInput))
                {
                    this.HandleYJoystick(ourInput);
                }
            }
            else
            {
                if (!this.HandleYJoystick(ourInput))
                {
                    this.HandleXJoystick(ourInput);
                }
            }

            if (ourInput.Buttons.A == ButtonState.Released)
            {
                this.PlayerZapper.StopFiring();
            }

            if ((atMazeJunction) && (!characterStopped))
            {
                switch (LastDirection) {
                    case Direction.DirectUp:
                        destinationColumn = this.CharacterColumn;
                        destinationRow = this.CharacterRow - 1;
                        break;
                    case Direction.DirectDown:
                        destinationColumn = this.CharacterColumn;
                        destinationRow = this.CharacterRow + 1;
                        break;
                    case Direction.DirectLeft:
                        destinationColumn = this.CharacterColumn - 1;
                        destinationRow = this.CharacterRow;
                        break;
                    case Direction.DirectRight:
                        destinationColumn = this.CharacterColumn + 1;
                        destinationRow = this.CharacterRow;
                        break;
                    default:
                        destinationColumn = 0;
                        destinationRow = 0;
                        break;
                }
                Debug.WriteLine(string.Format("Heading to {0}:{1}", destinationColumn, destinationRow));
            }
        }

        /// <summary>
        /// Handles movement in the X axis
        /// </summary>
        /// <param name="ourInput">The input state of our gamepad</param>
        /// <returns>True if the player did move in the X axis</returns>
        private bool HandleXJoystick(GamePadState ourInput)
        {
            if (ourInput.IsButtonDown(Buttons.DPadLeft) || ourInput.IsButtonDown(Buttons.LeftThumbstickLeft) )
            {
                if (westOk)
                {
                    LastDirection = Direction.DirectLeft;
                    characterStopped = false;
                    return true;
                }
                else if (this.characterStopped)
                {
                    LastDirection = Direction.DirectLeft;
                }
            }
            else if (ourInput.IsButtonDown(Buttons.DPadRight) || ourInput.IsButtonDown(Buttons.LeftThumbstickRight))
            {
                if (eastOk)
                {
                    LastDirection = Direction.DirectRight;
                    characterStopped = false;
                    return true;
                }
                else if (this.characterStopped)
                {
                    LastDirection = Direction.DirectRight;
                }
            }

            return false;
        }

        /// <summary>
        /// Handles movement in the Y axis
        /// </summary>
        /// <param name="ourInput">The input state of our gamepad</param>
        /// <returns>True if the player did move in the Y axis</returns>
        private bool HandleYJoystick(GamePadState ourInput)
        {
            if (ourInput.IsButtonDown(Buttons.DPadUp) || ourInput.IsButtonDown(Buttons.LeftThumbstickUp))
            {
                if (northOk)
                {
                    LastDirection = Direction.DirectUp;
                    characterStopped = false;
                    return true;
                }
                else if (this.characterStopped)
                {
                    LastDirection = Direction.DirectUp;
                }
            }
            else if (ourInput.IsButtonDown(Buttons.DPadDown) || ourInput.IsButtonDown(Buttons.LeftThumbstickDown))
            {
                if (southOk)
                {
                    LastDirection = Direction.DirectDown;
                    characterStopped = false;
                    return true;
                }
                else if (this.characterStopped)
                {
                    LastDirection = Direction.DirectDown;
                }
            }

            return false;
        }

        public void UpdateNetworkStatus()
        {
            GameInformation.Instance.GamePacketWriter.Write((byte)0x00);
            GameInformation.Instance.GamePacketWriter.Write((byte)((byte)this.LastDirection | (byte)(this.characterStopped ? 0x80 : 0x00)));
            GameInformation.Instance.GamePacketWriter.Write((float)this.CharacterSpeed);
            GameInformation.Instance.GamePacketWriter.Write((byte)destinationColumn);
            GameInformation.Instance.GamePacketWriter.Write((byte)destinationRow);
        }
    }
}
