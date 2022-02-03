//-----------------------------------------------------------------------------
// <copyright file="HarmlessLogoScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// The background screen sits behind all the other menu screens.
    /// It draws a background image that remains fixed in place regardless
    /// of whatever transitions the screens on top of it may be doing.
    /// </summary>
    public class HarmlessLogoScreen : GameScreen, IDisposable
    {
        #region Fields

        /// <summary>
        /// This holds the timespan for which the logo screen will be shown
        /// </summary>
        private readonly TimeSpan logoShowTime = TimeSpan.FromSeconds(5);

        /// <summary>
        /// This holds the temporary content manager we will use for the screen
        /// </summary>
        private ContentManager content;

        /// <summary>
        /// This holds the texture that will be shown for the logo
        /// </summary>
        private Texture2D logoTexture;

        /// <summary>
        /// This tracks the total time the logo has been shown
        /// </summary>
        private TimeSpan totalLogoTime = TimeSpan.Zero;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the HarmlessLogoScreen class.
        /// </summary>
        public HarmlessLogoScreen()
        {
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
        }

        /// <summary>
        /// Loads graphics content for this screen. The background texture is quite
        /// big, so we use our own local ContentManager to load it. This allows us
        /// to unload before going from the menus into the game itself, wheras if we
        /// used the shared ContentManager provided by the Game class, the content
        /// would remain loaded forever.
        /// </summary>
        public override void LoadContent()
        {
            if (this.content == null)
            {
                this.content = new ContentManager(ScreenManager.Game.Services, "Content");
            }

            this.logoTexture = this.content.Load<Texture2D>("MiscGraphics\\HarmlessLogo");
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
            this.content.Unload();
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Updates the logo screen.
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">Boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">Boolean set to true if some other screen covers us</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            if (!otherScreenHasFocus)
            {
                this.totalLogoTime += gameTime.ElapsedGameTime;
                if (this.totalLogoTime > this.logoShowTime)
                {
                    this.IsExiting = true;
                    ScreenManager.AddScreen(new TitleScreen());
                }
            }

            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
        }

        /// <summary>
        /// Draws the background screen.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
            Rectangle fullscreen = new Rectangle(0, 0, viewport.Width, viewport.Height);
            byte fade = TransitionAlpha;

            // XNA 4.0
            //spriteBatch.Begin(SpriteBlendMode.None);
            spriteBatch.Begin(SpriteSortMode.FrontToBack, BlendState.Opaque);

            spriteBatch.Draw(this.logoTexture, fullscreen, new Color(fade, fade, fade));

            spriteBatch.End();
        }

        #endregion

        #region IDisposable Members

        /// <summary>
        /// This is the public dispose method that disposes managed and any unmanaged resources
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// This is the internal dispose method that disposes the resources it has been instructed to
        /// </summary>
        /// <param name="disposing">Set to true if it is ok to dispose any managed resources</param>
        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.content.Dispose();
                this.logoTexture.Dispose();
            }
        }

        #endregion
    }
}
