//-----------------------------------------------------------------------------
// <copyright file="BackgroundScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft. All rights reserved.
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
    public class BackgroundScreen : GameScreen, IDisposable
    {
        #region Fields

        /// <summary>
        /// The content manager to load background screens with
        /// </summary>
        private ContentManager content;

        /// <summary>
        /// The texture we are going to be showing in the background
        /// </summary>
        private Texture2D backgroundTexture;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the BackgroundScreen class
        /// </summary>
        public BackgroundScreen()
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

            this.backgroundTexture = this.content.Load<Texture2D>("MiscGraphics\\title");
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
        /// Updates the background screen. Unlike most screens, this should not
        /// transition off even if it has been covered by another screen: it is
        /// supposed to be covered, after all! This overload forces the
        /// coveredByOtherScreen parameter to false in order to stop the base
        /// Update method wanting to transition off.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        /// <param name="otherScreenHasFocus">Is some other screen accepting input</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by some other screen</param>
        public override void Update(
            GameTime gameTime,
            bool otherScreenHasFocus,
            bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, false);
        }

        /// <summary>
        /// Draws the background screen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
            Rectangle fullscreen = new Rectangle(0, 0, viewport.Width, viewport.Height);
            byte fade = TransitionAlpha;

// XNA4.0 - Begin() takes new arguments - what sort mode is right?
//            spriteBatch.Begin(SpriteBlendMode.None);
            spriteBatch.Begin(SpriteSortMode.Texture, BlendState.Opaque);

            spriteBatch.Draw(
                this.backgroundTexture,
                fullscreen,
                new Color(fade, fade, fade));

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
            }
        }
        #endregion
    }
}
