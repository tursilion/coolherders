//-----------------------------------------------------------------------------
// <copyright file="SheepCharacter.cs" company="HarmlessLion">
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
    using CoolHerders.GameItems;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// This class tracks each sheep as it wanders the level
    /// </summary>
    internal class SheepCharacter : MovingCharacter
    {
        /// <summary>
        /// If the sheep is owned, which player does so
        /// </summary>
        public PlayerCharacter OwningPlayer;

        /// <summary>
        /// The offset to place on characters to make them lock to the maze grid correctly
        /// </summary>
        public new Vector2 DrawingOffset = new Vector2(16, 0);

        /// <summary>
        /// Current state of the sheep
        /// </summary>
        public SheepState CurrentState;

        /// <summary>
        /// How long should a sheep remain stunned for
        /// </summary>
        private static readonly TimeSpan stunnedDuration = TimeSpan.FromMilliseconds(750);

        /// <summary>
        /// How long should a sheep remain invincible for
        /// </summary>
        private static readonly TimeSpan invincibleDuration = TimeSpan.FromSeconds(2);

        /// <summary>
        /// Where are we in the position array?
        /// </summary>
        private int positionNumber;

        /// <summary>
        /// How much longer should the sheep remain in the current state for
        /// </summary>
        private TimeSpan stateTime;

        /// <summary>
        /// How much does a sheep need to be offset from it's drawing co-ordinates to look right
        /// </summary>
        private Vector2 offsetVector;

        /// <summary>
        /// Initializes a new instance of the SheepCharacter class
        /// </summary>
        /// <param name="parentComponent">The parent component that will draw this sheep</param>
        /// <param name="gamerClass">The gamer which is controlling this character, or the managing gamer if a computer controlled object</param>
        /// <param name="playerClassName">The class name of this sheep (probably Sheep)</param>
        /// <param name="subclass">The subclass name for this sheep</param>
        /// <param name="rowCounter">The row that this sheep will start on</param>
        /// <param name="columnCounter">The column that this sheep will start on</param>
        public SheepCharacter(MazeGameComponent parentComponent, NetworkGamer networkGamer, PlayerInformation playerInformation, int rowCounter, int columnCounter)
            : base(parentComponent, networkGamer, playerInformation, rowCounter, columnCounter)
        {
            this.CurrentState = SheepState.sheepInvincible;
            this.CheckCollisions = false;
            this.stateTime = invincibleDuration;
            this.CharacterDepth = 0.002f;
            this.offsetVector = new Vector2(24.0f, 20.0f);

            AudioEngine ourAudioEngine = ((CoolHerdersGame)this.parentComponent.GameScreen.ScreenManager.Game).GameAudioEngine;
            this.characterVoiceWaveBank = new WaveBank(ourAudioEngine, "Content\\Audio\\SoundEffects.xwb");
            this.characterVoiceSoundBank = new SoundBank(ourAudioEngine, "Content\\Audio\\FlossieVoice.xsb");
        }

        /// <summary>
        /// Holds the state of the sheep in the game world
        /// </summary>
        public enum SheepState
        {
            /// <summary>
            /// The sheep is roaming the level of it's own accord
            /// </summary>
            sheepRoaming,

            /// <summary>
            /// The sheep is following a player
            /// </summary>
            sheepFollowing,

            /// <summary>
            /// The sheep is stunned
            /// </summary>
            sheepStunned,

            /// <summary>
            /// The sheep is invincible.  It is roaming, and cannot be stunned.
            /// </summary>
            sheepInvincible,
        }

        /// <summary>
        /// Locks this sheep to a given player
        /// </summary>
        /// <param name="playerOwning">The player to follow</param>
        public void LockToPlayer(PlayerCharacter playerOwning)
        {
            this.OwningPlayer = playerOwning;
            this.CurrentState = SheepState.sheepFollowing;
            this.positionNumber = playerOwning.AddASheep(this);
            this.CheckCollisions = false;
            this.characterFrozen = false;
            this.characterVoiceSoundBank.PlayCue("flossie");
        }

        /// <summary>
        /// Stuns this sheep rendering it immobile
        /// </summary>
        public void Stun()
        {
            this.CurrentState = SheepState.sheepStunned;
            this.stateTime = SheepCharacter.stunnedDuration;
            this.characterFrozen = true;
        }

        /// <summary>
        /// Frees this sheep to run around the level
        /// </summary>
        public void RunFree()
        {
            this.CharacterRow = this.OwningPlayer.CharacterRow;
            this.CharacterColumn = this.OwningPlayer.CharacterColumn;
            this.CharacterPosition.Y = this.CharacterRow * 32.0f;
            this.CharacterPosition.X = this.CharacterColumn * 32.0f;
            this.CurrentState = SheepState.sheepInvincible;
            this.stateTime = SheepCharacter.invincibleDuration;
            this.OwningPlayer = null;
            this.CharacterSpeed = 225.0f;
        }

        /// <summary>
        /// Draws a single player sprite
        /// </summary>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="transitionColor">The color number that should be applied to help cope with any transition effects</param>
        /// <param name="alphaChannel">An additional alpha channel to use when drawing the sheep</param>
        public override void Draw(SpriteBatch mazeSpriteBatch, byte transitionColor, byte alphaChannel)
        {
            if (this.CurrentState == SheepState.sheepInvincible)
            {
                if (transitionColor > 200)
                {
                    transitionColor = 200;
                }

                if (GameInformation.Instance.WorldScreen.SheepAreGhosted)
                {
                    base.Draw(mazeSpriteBatch, transitionColor, 128);
                }
                else
                {
                    base.Draw(mazeSpriteBatch, transitionColor, 200);
                }
            }
            else if (GameInformation.Instance.WorldScreen.SheepAreGhosted)
            {
                base.Draw(mazeSpriteBatch, transitionColor, 160);
            }
            else
            {
                base.Draw(mazeSpriteBatch, transitionColor, 255);
            }
        }

        /// <summary>
        /// Updates this sheep within the gameworld
        /// </summary>
        /// <param name="gameTime">The current GameTime of this game</param>
        /// <param name="obstructingObjects">Objects which we may not move through</param>
        /// <param name="collectableObjects">Objects which we may collect</param>
        internal override void Update(GameTime gameTime, ICollidable[] obstructingObjects, ICollidable[] collectableObjects)
        {
            if (this.CurrentState == SheepState.sheepStunned)
            {
                this.stateTime -= gameTime.ElapsedGameTime;
                if (this.stateTime.TotalSeconds <= 0)
                {
                    this.CurrentState = SheepState.sheepRoaming;
                    this.characterFrozen = false;
                }

                return;
            }

            Vector2 previousCharPosition = this.CharacterPosition;

            base.Update(gameTime, obstructingObjects, collectableObjects);

            switch (this.CurrentState)
            {
                case SheepState.sheepFollowing:
                    if (this.CharacterPosition != this.OwningPlayer.CharacterPosition)
                    {
                        if ((this.LastDirection == Direction.DirectLeft) || (this.LastDirection == Direction.DirectUp))
                        {
                            this.CharacterDepth = this.CharacterDepth + ((float)this.positionNumber / 100000.0f) - 0.005f;
                        }
                        else
                        {
                            this.CharacterDepth = this.CharacterDepth - ((float)this.positionNumber / 100000.0f) - 0.005f;
                        }
                    }
                    else
                    {
                        this.CharacterDepth = this.OwningPlayer.CharacterDepth - 0.000001f;
                    }

                    break;
                case SheepState.sheepRoaming:
                    if (CheckCollision(obstructingObjects))
                    {
                        this.CharacterPosition = previousCharPosition;
                        this.CollisionBox = new BoundingBox(new Vector3(this.CharacterPosition.X, this.CharacterPosition.Y, 0.0f), new Vector3(this.CharacterPosition.X + 24, this.CharacterPosition.Y + 16, 0.0f));
                        TurnCharacterReverse();
                    }

                    break;
                case SheepState.sheepInvincible:
                    this.stateTime -= gameTime.ElapsedGameTime;
                    if (this.stateTime.TotalSeconds <= 0)
                    {
                        this.CurrentState = SheepState.sheepRoaming;
                        this.CheckCollisions = true;
                        this.CharacterSpeed = 150.0f;
                    }

                    break;
            }
        }

        /// <summary>
        /// Called when the sheep needs to change direction.
        /// Put some real AI here.
        /// </summary>
        /// <param name="playerIndex">The player index that is controlling us (presently unused)</param>
        /// <param name="atMazeJunction">Is this sheep at a maze junction right now</param>
        /// <param name="collectableObjects">Any objects within the maze that can be picked up by the sheep</param>
        protected override void HandleMovementChange(PlayerIndex playerIndex, bool atMazeJunction, ICollidable[] collectableObjects)
        {
            this.characterStopped = true;
            SheepState tempState = this.CurrentState;
            if (tempState == SheepState.sheepInvincible)
            {
                tempState = SheepState.sheepRoaming;
            }

            switch (tempState)
            {
                case SheepState.sheepStunned:
                    break;
                case SheepState.sheepRoaming:
                    int numberExits = parentComponent.RandomNumber.Next(1, 3);

                    if (numberExits == 1)
                    {
                        if (!CanGoForward())
                        {
                            if (!TurnCharacterClockwise())
                            {
                                if (!TurnCharacterCounterClockwise())
                                {
                                    TurnCharacterReverse();
                                }
                            }
                        }
                    }

                    if (numberExits == 2)
                    {
                        if (!TurnCharacterCounterClockwise())
                        {
                            if (!TurnCharacterClockwise())
                            {
                                if (!CanGoForward())
                                {
                                    TurnCharacterReverse();
                                }
                            }
                        }
                    }

                    if (numberExits == 3)
                    {
                        if (!TurnCharacterClockwise())
                        {
                            if (!CanGoForward())
                            {
                                if (!TurnCharacterCounterClockwise())
                                {
                                    TurnCharacterReverse();
                                }
                            }
                        }
                    }

                    this.characterStopped = false;
                    break;

                case SheepState.sheepFollowing:
                    this.CharacterPosition = this.OwningPlayer.GetPreviousPosition(this.positionNumber + 10);
                    this.LastDirection = this.OwningPlayer.GetPreviousDirection(this.positionNumber + 10);
                    break;
            }
        }
    }
}

