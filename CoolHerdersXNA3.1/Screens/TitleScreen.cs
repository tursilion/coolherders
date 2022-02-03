//-----------------------------------------------------------------------------
// <copyright file="TitleScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using CoolHerders.Housekeeping;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    #endregion

    /// <summary>
    /// The background screen sits behind all the other menu screens.
    /// It draws a background image that remains fixed in place regardless
    /// of whatever transitions the screens on top of it may be doing.
    /// </summary>
    internal class TitleScreen : GameScreen, IDisposable
    {
        #region Fields

        /// <summary>
        /// The message to print when prompting the user to press start
        /// </summary>
        private readonly string pressStartMessage = "Press Start!";

        /// <summary>
        /// This tracks our temporary content manager
        /// </summary>
        private ContentManager content;

        /// <summary>
        /// This holds the big font for the 'press start' text
        /// </summary>
        private SpriteFont bigFont;

        /// <summary>
        /// This tracks the texture we will use to display the title
        /// </summary>
        private Texture2D titleTexture;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the TitleScreen class
        /// </summary>
        public TitleScreen()
        {
#if DEBUG
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
#else
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
#endif
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

            this.titleTexture = this.content.Load<Texture2D>("MiscGraphics\\title");
            this.bigFont = this.content.Load<SpriteFont>("TimesNewRoman");

            ((CoolHerdersGame)ScreenManager.Game).PlayBackgroundAudioCue("TitleBackground");
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
            this.content.Unload();
        }

        #endregion

        /// <summary>
        /// Responds to user input, accepting or cancelling the message box.
        /// </summary>
        /// <param name="input">The input state currently present in the system</param>
        public override void HandleInput(InputState input)
        {
            for (PlayerIndex playerIndex = PlayerIndex.One; playerIndex <= PlayerIndex.Four; playerIndex++)
            {
                if (ScreenState == ScreenState.Active)
                {
                    if ((input.IsNewButtonPress(Buttons.Start, playerIndex)) || (input.IsNewKeyPress(Keys.Enter, playerIndex)))
                    {
                        GameInformation.Instance.MasterPlayerIndex = playerIndex;
                        LoadingScreen.Load(ScreenManager, false, new BackgroundScreen(), new MainMenuScreen());
                    }
                }
            }
        }

        #region Update and Draw

        /// <summary>
        /// Shows the title screen and waits for the player to press the 'start' button to assign the primary controller
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">Boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">Boolean set to true if some other screen covers us</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
        }

        /// <summary>
        /// Draws the background screen.
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
            Rectangle fullscreen = new Rectangle(0, 0, viewport.Width, viewport.Height);

            byte fade = TransitionAlpha;

            // XNA4.0
            //spriteBatch.Begin(SpriteBlendMode.AlphaBlend);
            spriteBatch.Begin();

            spriteBatch.Draw(this.titleTexture, fullscreen, new Color(fade, fade, fade));

            if (TransitionAlpha == 255)
            {
                Vector2 fontOrigin = this.bigFont.MeasureString(this.pressStartMessage) / 2;
                Vector2 fontStart = new Vector2(ScreenManager.GraphicsDevice.Viewport.Width / 2, ScreenManager.GraphicsDevice.Viewport.Height * 0.75f);
                Vector2 fontStart2 = new Vector2(fontStart.X + 1, fontStart.Y + 1);
                spriteBatch.DrawString(this.bigFont, this.pressStartMessage, fontStart2, Color.Black, 0, fontOrigin, 1, SpriteEffects.None, 0);
                spriteBatch.DrawString(this.bigFont, this.pressStartMessage, fontStart, Color.LightSlateGray, 0, fontOrigin, 1, SpriteEffects.None, 0);
            }

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
                this.titleTexture.Dispose();
            }
        }
        #endregion
    }
}
