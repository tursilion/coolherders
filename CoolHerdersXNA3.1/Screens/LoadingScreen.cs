//-----------------------------------------------------------------------------
// LoadingScreen.cs
//
// Microsoft XNA Community Game Platform
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#region Using Statements
using System;
using System.Threading;
using System.Diagnostics;
using CoolHerders.Properties;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Net;
#endregion

namespace CoolHerders
{
    /// <summary>
    /// The loading screen coordinates transitions between the menu system and the
    /// game itself. Normally one screen will transition off at the same time as
    /// the next screen is transitioning on, but for larger transitions that can
    /// take a longer time to load their data, we want the menu system to be entirely
    /// gone before we start loading the game. This is done as follows:
    /// 
    /// - Tell all the existing screens to transition off.
    /// - Activate a loading screen, which will transition on at the same time.
    /// - The loading screen watches the state of the previous screens.
    /// - When it sees they have finished transitioning off, it activates the real
    ///   next screen, which may take a long time to load its data. The loading
    ///   screen will be the only thing displayed while this load is taking place.
    /// </summary>
    class LoadingScreen : GameScreen
    {
        #region Fields

        bool loadingIsSlow;
        bool otherScreensAreGone;

        GameScreen[] screensToLoad;

        Thread backgroundThread;
        EventWaitHandle backgroundThreadExit;

        GraphicsDevice graphicsDevice;
        NetworkSession networkSession;
        IMessageDisplay messageDisplay;

        GameTime loadStartTime;
        TimeSpan loadAnimationTimer;

        Texture2D backgroundTexture;
        Texture2D sheepTexture;

        Vector2[] sheepPositions = new Vector2[30];
        bool[] sheepDirections = new bool[30];

        SpriteFont bigFont;

        ContentManager content;

        #endregion

        #region Initialization


        /// <summary>
        /// The constructor is private: loading screens should
        /// be activated via the static Load method instead.
        /// </summary>
        private LoadingScreen(ScreenManager screenManager, bool loadingIsSlow,
                              GameScreen[] screensToLoad)
        {
            this.loadingIsSlow = loadingIsSlow;
            this.screensToLoad = screensToLoad;

            TransitionOnTime = TimeSpan.FromSeconds(0.5);

            // If this is going to be a slow load operation, create a background
            // thread that will update the network session and draw the load screen
            // animation while the load is taking place.
            if (loadingIsSlow)
            {
                for (int counter = 0; counter < this.sheepPositions.Length; counter++)
                {
                    this.sheepPositions[counter].X = -20 + counter * 50;
                    this.sheepPositions[counter].Y = 500 + (float)Math.Cos(counter) * 10;
                    this.sheepDirections[counter] = true;
                }

                backgroundThread = new Thread(BackgroundWorkerThread);
                backgroundThreadExit = new ManualResetEvent(false);

                graphicsDevice = screenManager.GraphicsDevice;

                // Look up some services that will be used by the background thread.
                IServiceProvider services = screenManager.Game.Services;

                networkSession = (NetworkSession)services.GetService(
                                                            typeof(NetworkSession));

                messageDisplay = (IMessageDisplay)services.GetService(
                                                            typeof(IMessageDisplay));
            }
        }


        /// <summary>
        /// Activates the loading screen.
        /// </summary>
        public static void Load(ScreenManager screenManager, bool loadingIsSlow,
                                params GameScreen[] screensToLoad)
        {
            // Tell all the current screens to transition off.
            foreach (GameScreen screen in screenManager.GetScreens())
                screen.ExitScreen();

            // Create and activate the loading screen.
            LoadingScreen loadingScreen = new LoadingScreen(screenManager,
                                                            loadingIsSlow,
                                                            screensToLoad);

            screenManager.AddScreen(loadingScreen);
        }


        #endregion

        public override void LoadContent()
        {
            base.LoadContent();
            if (this.content == null)
            {
                this.content = new ContentManager(ScreenManager.Game.Services, "Content");
            }

            this.backgroundTexture = this.content.Load<Texture2D>("Story\\ChapterHeaders\\grass tile");
            this.sheepTexture = this.content.Load<Texture2D>("Characters\\Sheep\\Sheep5");
            this.bigFont = this.content.Load<SpriteFont>("TimesNewRoman");
        }

        #region Update and Draw


        /// <summary>
        /// Updates the loading screen.
        /// </summary>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus,
                                                       bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            // If all the previous screens have finished transitioning
            // off, it is time to actually perform the load.
            if (otherScreensAreGone)
            {
                // Start up the background thread, which will update the network
                // session and draw the animation while we are loading.
                if (backgroundThread != null)
                {
                    loadStartTime = gameTime;
                    backgroundThread.Start();
                }

                // Perform the load operation.
                ScreenManager.RemoveScreen(this);

                foreach (GameScreen screen in screensToLoad)
                {
                    if (screen != null)
                    {
                        ScreenManager.AddScreen(screen);
                    }
                }

                // Signal the background thread to exit, then wait for it to do so.
                if (backgroundThread != null)
                {
                    backgroundThreadExit.Set();
                    backgroundThread.Join();
                }

                // Once the load has finished, we use ResetElapsedTime to tell
                // the  game timing mechanism that we have just finished a very
                // long frame, and that it should not try to catch up.
                ScreenManager.Game.ResetElapsedTime();
            }
        }


        /// <summary>
        /// Draws the loading screen.
        /// </summary>
        public override void Draw(GameTime gameTime)
        {
            // If we are the only active screen, that means all the previous screens
            // must have finished transitioning off. We check for this in the Draw
            // method, rather than in Update, because it isn't enough just for the
            // screens to be gone: in order for the transition to look good we must
            // have actually drawn a frame without them before we perform the load.
            if ((ScreenState == ScreenState.Active) &&
                (ScreenManager.GetScreens().Length == 1))
            {
                otherScreensAreGone = true;
            }

            // The gameplay screen takes a while to load, so we display a loading
            // message while that is going on, but the menus load very quickly, and
            // it would look silly if we flashed this up for just a fraction of a
            // second while returning from the game to the menus. This parameter
            // tells us how long the loading is going to take, so we know whether
            // to bother drawing the message.
            if (loadingIsSlow)
            {
                SpriteBatch spriteBatch = ScreenManager.SpriteBatch;

                string message = Resources.Loading;


                // Create a rectangle that is centered horizontally, and at the top of the screen vertically
                Rectangle fullScreenArea = new Rectangle(0, 0, 1280, 720);

                Rectangle sheepTextureSelection = new Rectangle(2 * 48, 4 * 48, 48, 48);

                // Center the text in the viewport.
                Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
                Vector2 viewportSize = new Vector2(viewport.Width, viewport.Height);
                Vector2 textSize = bigFont.MeasureString(message);
                Vector2 textPosition = (viewportSize - textSize) / 2;
                textPosition.Y -= 150.0f;

                Color color = new Color(255, 255, 255, TransitionAlpha);

                // Animate the number of dots after our "Loading..." message.
                loadAnimationTimer += gameTime.ElapsedGameTime;

                // Draw the text.
                spriteBatch.Begin();
                spriteBatch.Draw(this.backgroundTexture, fullScreenArea, new Color(255, 255, 255, TransitionAlpha));
                spriteBatch.DrawString(this.bigFont, message, textPosition, color);
                for (int counter = 0; counter < this.sheepPositions.Length; counter++)
                {
                    this.sheepPositions[counter].Y = 500 + (float)Math.Cos(counter + (loadAnimationTimer.TotalMilliseconds / 125)) * 75;
                    spriteBatch.Draw(this.sheepTexture, this.sheepPositions[counter], sheepTextureSelection, Color.White, (float)loadAnimationTimer.TotalSeconds, Vector2.Zero, 1.0f + ((float)Math.Cos(loadAnimationTimer.TotalSeconds) / 2), SpriteEffects.None, 0.0f);
                }
                spriteBatch.End();
            }
        }


        #endregion

        #region Background Thread


        /// <summary>
        /// Worker thread draws the loading animation and updates the network
        /// session while the load is taking place.
        /// </summary>
        void BackgroundWorkerThread()
        {
            long lastTime = Stopwatch.GetTimestamp();

            // EventWaitHandle.WaitOne will return true if the exit signal has
            // been triggered, or false if the timeout has expired. We use the
            // timeout to update at regular intervals, then break out of the
            // loop when we are signalled to exit.
            while (!backgroundThreadExit.WaitOne(1000 / 30, false))
            {
                GameTime gameTime = GetGameTime(ref lastTime);

                DrawLoadAnimation(gameTime);

                UpdateNetworkSession();
            }
        }


        /// <summary>
        /// Works out how long it has been since the last background thread update.
        /// </summary>
        GameTime GetGameTime(ref long lastTime)
        {
            long currentTime = Stopwatch.GetTimestamp();
            long elapsedTicks = currentTime - lastTime;
            lastTime = currentTime;

            TimeSpan elapsedTime = TimeSpan.FromTicks(elapsedTicks *
                                                      TimeSpan.TicksPerSecond /
                                                      Stopwatch.Frequency);

// XNA4.0 - TotalRealTime obsoleted (use TotalGameTime as a hack... ?)
//            return new GameTime(loadStartTime.TotalRealTime + elapsedTime, elapsedTime,
//                                loadStartTime.TotalGameTime + elapsedTime, elapsedTime);
            return new GameTime(loadStartTime.TotalGameTime + elapsedTime, elapsedTime);
        }


        /// <summary>
        /// Calls directly into our Draw method from the background worker thread,
        /// so as to update the load animation in parallel with the actual loading.
        /// </summary>
        void DrawLoadAnimation(GameTime gameTime)
        {
            if ((graphicsDevice == null) || graphicsDevice.IsDisposed)
                return;

            try
            {
                graphicsDevice.Clear(Color.Black);

                // Draw the loading screen.
                Draw(gameTime);

                // If we have a message display component, we want to display
                // that over the top of the loading screen, too.
                if (messageDisplay != null)
                {
                    messageDisplay.Update(gameTime);
                    messageDisplay.Draw(gameTime);
                }

                graphicsDevice.Present();
            }
            catch
            {
                // If anything went wrong (for instance the graphics device was lost
                // or reset) we don't have any good way to recover while running on a
                // background thread. Setting the device to null will stop us from
                // rendering, so the main game can deal with the problem later on.
                graphicsDevice = null;
            }
        }


        /// <summary>
        /// Updates the network session from the background worker thread, to avoid
        /// disconnecting due to network timeouts even if loading takes a long time.
        /// </summary>
        void UpdateNetworkSession()
        {
            if ((networkSession == null) ||
                (networkSession.SessionState == NetworkSessionState.Ended))
                return;

            try
            {
                networkSession.Update();
            }
            catch
            {
                // If anything went wrong, we don't have a good way to report that
                // error while running on a background thread. Setting the session to
                // null will stop us from updating it, so the main game can deal with
                // the problem later on.
                networkSession = null;
            }
        }


        #endregion
    }
}
