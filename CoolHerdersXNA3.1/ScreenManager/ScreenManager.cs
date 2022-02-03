//-----------------------------------------------------------------------------
// <copyright file="ScreenManager.cs" company="Microsoft">
//  Microsoft XNA Community Game Platform
//  Copyright (C) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// The screen manager is a component which manages one or more GameScreen
    /// instances. It maintains a stack of screens, calls their Update and Draw
    /// methods at the appropriate times, and automatically routes input to the
    /// topmost active screen.
    /// </summary>
    public class ScreenManager : DrawableGameComponent
    {
        #region Fields

        /// <summary>
        /// A list of screens that the screen manager is responsible for
        /// </summary>
        private List<GameScreen> screens = new List<GameScreen>();

        /// <summary>
        /// A list of screens that need to be updated on this pass
        /// </summary>
        private List<GameScreen> screensToUpdate = new List<GameScreen>();

        /// <summary>
        /// The state of all inputs into the game
        /// </summary>
        private InputState input = new InputState();

        /// <summary>
        /// A sprite batch used for managing screens
        /// </summary>
        private SpriteBatch spriteBatch;

        /// <summary>
        /// A generic font that all screens can use
        /// </summary>
        private SpriteFont font;

        /// <summary>
        /// A generic blank texture shared betwen screens
        /// </summary>
        private Texture2D blankTexture;

        /// <summary>
        /// Tracks if the screen manager is initialized
        /// </summary>
        private bool isInitialized;

        /// <summary>
        /// Tracks if the screen tracing is enabled
        /// </summary>
        private bool traceEnabled;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the ScreenManager class.
        /// </summary>
        /// <param name="game">The game object to which the screen manager component should join.</param>
        public ScreenManager(Game game)
            : base(game)
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets a default SpriteBatch shared by all the screens. This saves
        /// each screen having to bother creating their own local instance.
        /// </summary>
        public SpriteBatch SpriteBatch
        {
            get { return this.spriteBatch; }
        }

        /// <summary>
        /// Gets a default font shared by all the screens. This saves
        /// each screen having to bother loading their own local copy.
        /// </summary>
        public SpriteFont Font
        {
            get { return this.font; }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the manager prints out a list of all the screens
        /// each time it is updated. This can be useful for making sure
        /// everything is being added and removed at the right times.
        /// </summary>
        public bool TraceEnabled
        {
            get { return this.traceEnabled; }
            set { this.traceEnabled = value; }
        }

        /// <summary>
        /// Gets a blank texture shared across all screens
        /// </summary>
        public Texture2D BlankTexture
        {
            get
            {
                return this.blankTexture;
            }
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Allows each screen to run logic.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        public override void Update(GameTime gameTime)
        {
            // Read the keyboard and gamepad.
            this.input.Update();

#if DEBUG
            if (this.input.CurrentKeyboardStates[0].IsKeyDown(Microsoft.Xna.Framework.Input.Keys.Escape))
            {
                Game.Exit();
            }
#endif

            // Make a copy of the master screen list, to avoid confusion if
            // the process of updating one screen adds or removes others.
            this.screensToUpdate.Clear();

            foreach (GameScreen screen in this.screens)
            {
                this.screensToUpdate.Add(screen);
            }

            bool otherScreenHasFocus = !Game.IsActive;
            bool coveredByOtherScreen = false;

            // Loop as long as there are screens waiting to be updated.
            while (this.screensToUpdate.Count > 0)
            {
                // Pop the topmost screen off the waiting list.
                GameScreen screen = this.screensToUpdate[this.screensToUpdate.Count - 1];

                this.screensToUpdate.RemoveAt(this.screensToUpdate.Count - 1);

                // Update the screen.
                screen.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

                if (screen.ScreenState == ScreenState.TransitionOn ||
                    screen.ScreenState == ScreenState.Active)
                {
                    // If this is the first active screen we came across,
                    // give it a chance to handle input.
                    if (!otherScreenHasFocus)
                    {
                        screen.HandleInput(this.input);

                        otherScreenHasFocus = true;
                    }

                    // If this is an active non-popup, inform any subsequent
                    // screens that they are covered by it.
                    if (!screen.IsPopup)
                    {
                        coveredByOtherScreen = true;
                    }
                }
            }

            // Print debug trace?
            if (this.traceEnabled)
            {
                this.TraceScreens();
            }
        }

        /// <summary>
        /// Tells each screen to draw itself.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            foreach (GameScreen screen in this.screens)
            {
                if (screen.ScreenState == ScreenState.Hidden)
                {
                    continue;
                }

                screen.Draw(gameTime);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Adds a new screen to the screen manager.
        /// </summary>
        /// <param name="screen">A GameScreen class that is to be put under management</param>
        public void AddScreen(GameScreen screen)
        {
            screen.ScreenManager = this;
            screen.IsExiting = false;

            // If we have a graphics device, tell the screen to load content.
            if (this.isInitialized)
            {
                screen.LoadContent();
            }

            this.screens.Add(screen);
        }

        /// <summary>
        /// Removes a screen from the screen manager. You should normally
        /// use GameScreen.ExitScreen instead of calling this directly, so
        /// the screen can gradually transition off rather than just being
        /// instantly removed.
        /// </summary>
        /// <param name="screen">The Screen class to remove from management</param>
        public void RemoveScreen(GameScreen screen)
        {
            // If we have a graphics device, tell the screen to unload content.
            if (this.isInitialized)
            {
                screen.UnloadContent();
            }

            this.screens.Remove(screen);
            this.screensToUpdate.Remove(screen);
        }

        /// <summary>
        /// Expose an array holding all the screens. We return a copy rather
        /// than the real master list, because screens should only ever be added
        /// or removed using the AddScreen and RemoveScreen methods.
        /// </summary>
        /// <returns>An array of GameScreen classes that are under management</returns>
        public GameScreen[] GetScreens()
        {
            return this.screens.ToArray();
        }

        /// <summary>
        /// Helper draws a translucent black fullscreen sprite, used for fading
        /// screens in and out, and for darkening the background behind popups.
        /// </summary>
        /// <param name="alpha">Integer containing the alpha value to apply</param>
        public void FadeBackBufferToBlack(int alpha)
        {
            Viewport viewport = GraphicsDevice.Viewport;

            this.spriteBatch.Begin();

            this.spriteBatch.Draw(
                             this.blankTexture,
                             new Rectangle(0, 0, viewport.Width, viewport.Height),
                             new Color(0, 0, 0, (byte)alpha));

            this.spriteBatch.End();
        }

        #endregion

        /// <summary>
        /// Initializes the screen manager component.
        /// </summary>
        public override void Initialize()
        {
            base.Initialize();

            Game.Services.AddService(typeof(IInputState), this.input);

            this.isInitialized = true;
        }

        /// <summary>
        /// Load your graphics content.
        /// </summary>
        protected override void LoadContent()
        {
            // Load content belonging to the screen manager.
            ContentManager content = Game.Content;

            this.spriteBatch = new SpriteBatch(GraphicsDevice);
            this.blankTexture = content.Load<Texture2D>("MiscGraphics\\blank");

            this.font = content.Load<SpriteFont>("Arial");

            // Tell each of the screens to load their content.
            foreach (GameScreen screen in this.screens)
            {
                screen.LoadContent();
            }
        }

        /// <summary>
        /// Unload your graphics content.
        /// </summary>
        protected override void UnloadContent()
        {
            // Tell each of the screens to unload their content.
            foreach (GameScreen screen in this.screens)
            {
                screen.UnloadContent();
            }
        }

        /// <summary>
        /// Prints a list of all the screens, for debugging.
        /// </summary>
        private void TraceScreens()
        {
            List<string> screenNames = new List<string>();

            foreach (GameScreen screen in this.screens)
            {
                screenNames.Add(screen.GetType().Name);
            }

            Trace.WriteLine(string.Join(", ", screenNames.ToArray()));
        }
    }
}
