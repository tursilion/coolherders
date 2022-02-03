//-----------------------------------------------------------------------------
// <copyright file="GameScreen.cs" company="Microsoft">
//  Microsoft XNA Community Game Platform
//  Copyright (C) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using System;
    using Microsoft.Xna.Framework;
    #endregion

    /// <summary>
    /// Enum describes the screen transition state.
    /// </summary>
    public enum ScreenState
    {
        /// <summary>
        /// The screen is transitioning to the on state
        /// </summary>
        TransitionOn,

        /// <summary>
        /// The screen is fully active
        /// </summary>
        Active,

        /// <summary>
        /// The screen is transitioning to the off state
        /// </summary>
        TransitionOff,

        /// <summary>
        /// The screen is hidden
        /// </summary>
        Hidden,
    }

    /// <summary>
    /// A screen is a single layer that has update and draw logic, and which
    /// can be combined with other layers to build up a complex menu system.
    /// For instance the main menu, the options menu, the "are you sure you
    /// want to quit" message box, and the main game itself are all implemented
    /// as screens.
    /// </summary>
    public abstract class GameScreen
    {
        #region Fields
        /// <summary>
        /// This tracks if the screen is a popup
        /// </summary>
        private bool isPopup;

        /// <summary>
        /// This tracks the time the transition takes to come in
        /// </summary>
        private TimeSpan transitionOnTime = TimeSpan.Zero;

        /// <summary>
        /// This tracks the screen manager that is managing this screen
        /// </summary>
        private ScreenManager screenManager;

        /// <summary>
        /// This tracks the time the transition takes to go out
        /// </summary>
        private TimeSpan transitionOffTime = TimeSpan.Zero;

        /// <summary>
        /// This tracks the position of the transition (1 = fully in)
        /// </summary>
        private float transitionPosition = 1;

        /// <summary>
        /// This tracks the state of the current screen
        /// </summary>
        private ScreenState screenState = ScreenState.TransitionOn;

        /// <summary>
        /// This tracks if the screen is exiting
        /// </summary>
        private bool isExiting;

        /// <summary>
        /// This tracks if some other screen has focus
        /// </summary>
        private bool otherScreenHasFocus;
        #endregion

        #region Properties

        /// <summary>
        /// Gets a value indicating whether the screen is only a small
        /// popup, in which case screens underneath it do not need to bother
        /// transitioning off.
        /// </summary>
        public bool IsPopup
        {
            get { return this.isPopup; }
            protected set { this.isPopup = value; }
        }

        /// <summary>
        /// Gets how long the screen takes to
        /// transition on when it is activated.
        /// </summary>
        public TimeSpan TransitionOnTime
        {
            get { return this.transitionOnTime; }
            protected set { this.transitionOnTime = value; }
        }

        /// <summary>
        /// Gets how long the screen takes to
        /// transition off when it is deactivated.
        /// </summary>
        public TimeSpan TransitionOffTime
        {
            get { return this.transitionOffTime; }
            protected set { this.transitionOffTime = value; }
        }

        /// <summary>
        /// Gets the current position of the screen transition, ranging
        /// from zero (fully active, no transition) to one (transitioned
        /// fully off to nothing).
        /// </summary>
        public float TransitionPosition
        {
            get { return this.transitionPosition; }
            protected set { this.transitionPosition = value; }
        }

        /// <summary>
        /// Gets the current alpha of the screen transition, ranging
        /// from 255 (fully active, no transition) to 0 (transitioned
        /// fully off to nothing).
        /// </summary>
        public byte TransitionAlpha
        {
            get
            {
                return (byte)(255 - (this.TransitionPosition * 255));
            }
        }

        /// <summary>
        /// Gets the current screen transition state.
        /// </summary>
        public ScreenState ScreenState
        {
            get { return this.screenState; }
            protected set { this.screenState = value; }
        }

        /// <summary>
        /// Gets a value indicating whether the screen is exiting for real:
        /// if set, the screen will automatically remove itself as soon as the
        /// transition finishes.
        /// </summary>
        public bool IsExiting
        {
            get { return this.isExiting; }
            protected internal set { this.isExiting = value; }
        }

        /// <summary>
        /// Gets a value indicating whether this screen is active and can respond to user input.
        /// </summary>
        public bool IsActive
        {
            get
            {
                return !this.otherScreenHasFocus &&
                       (this.screenState == ScreenState.TransitionOn ||
                        this.screenState == ScreenState.Active);
            }
        }

        /// <summary>
        /// Gets the manager that this screen belongs to.
        /// </summary>
        public ScreenManager ScreenManager
        {
            get { return this.screenManager; }
            internal set { this.screenManager = value; }
        }

        #endregion

        #region Initialization

        /// <summary>
        /// Load graphics content for the screen.
        /// </summary>
        public virtual void LoadContent()
        {
        }

        /// <summary>
        /// Unload content for the screen.
        /// </summary>
        public virtual void UnloadContent()
        {
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Allows the screen to run logic, such as updating the transition position.
        /// Unlike HandleInput, this method is called regardless of whether the screen
        /// is active, hidden, or in the middle of a transition.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">A boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">A boolean set to true if this screen is visually covered by some other screen</param>
        public virtual void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            this.otherScreenHasFocus = otherScreenHasFocus;

            if (this.isExiting)
            {
                // If the screen is going away to die, it should transition off.
                this.screenState = ScreenState.TransitionOff;

                if (!this.UpdateTransition(gameTime, this.transitionOffTime, 1))
                {
                    // When the transition finishes, remove the screen.
                    this.ScreenManager.RemoveScreen(this);
                }
            }
            else if (coveredByOtherScreen)
            {
                // If the screen is covered by another, it should transition off.
                if (this.UpdateTransition(gameTime, this.transitionOffTime, 1))
                {
                    // Still busy transitioning.
                    this.screenState = ScreenState.TransitionOff;
                }
                else
                {
                    // Transition finished!
                    this.screenState = ScreenState.Hidden;
                }
            }
            else
            {
                // Otherwise the screen should transition on and become active.
                if (this.UpdateTransition(gameTime, this.transitionOnTime, -1))
                {
                    // Still busy transitioning.
                    this.screenState = ScreenState.TransitionOn;
                }
                else
                {
                    // Transition finished!
                    this.screenState = ScreenState.Active;
                }
            }
        }

        /// <summary>
        /// Allows the screen to handle user input. Unlike Update, this method
        /// is only called when the screen is active, and not when some other
        /// screen has taken the focus.
        /// </summary>
        /// <param name="input">An InputState class which the recent input for processing</param>
        public virtual void HandleInput(InputState input)
        {
        }

        /// <summary>
        /// This is called when the screen should draw itself.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time within the game</param>
        public virtual void Draw(GameTime gameTime)
        {
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Tells the screen to go away. Unlike ScreenManager.RemoveScreen, which
        /// instantly kills the screen, this method respects the transition timings
        /// and will give the screen a chance to gradually transition off.
        /// </summary>
        public void ExitScreen()
        {
            if (this.TransitionOffTime == TimeSpan.Zero)
            {
                // If the screen has a zero transition time, remove it immediately.
                ScreenManager.RemoveScreen(this);
            }
            else
            {
                // Otherwise flag that it should transition off and then exit.
                this.isExiting = true;
            }
        }

        #endregion

        /// <summary>
        /// Helper for updating the screen transition position.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        /// <param name="time">A TimeSpan indicating the span of time over which the transition is to take place</param>
        /// <param name="direction">A 0 if the transition is going in, or a 1 if the transition is going out</param>
        /// <returns>A boolean which is true if the transition is still occuring</returns>
        private bool UpdateTransition(GameTime gameTime, TimeSpan time, int direction)
        {
            // How much should we move by?
            float transitionDelta;

            if (time == TimeSpan.Zero)
            {
                transitionDelta = 1;
            }
            else
            {
                transitionDelta = (float)(gameTime.ElapsedGameTime.TotalMilliseconds /
                                          time.TotalMilliseconds);
            }

            // Update the transition position.
            this.transitionPosition += transitionDelta * direction;

            // Did we reach the end of the transition?
            if ((this.transitionPosition <= 0) || (this.transitionPosition >= 1))
            {
                this.transitionPosition = MathHelper.Clamp(this.transitionPosition, 0, 1);
                return false;
            }

            // Otherwise we are still busy transitioning.
            return true;
        }
    }
}
