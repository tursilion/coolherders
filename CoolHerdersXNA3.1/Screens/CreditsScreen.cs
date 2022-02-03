//-----------------------------------------------------------------------------
// <copyright file="CreditsScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    #endregion

    /// <summary>
    /// The background screen sits behind all the other menu screens.
    /// It draws a background image that remains fixed in place regardless
    /// of whatever transitions the screens on top of it may be doing.
    /// </summary>
    internal class CreditsScreen : GameScreen
    {
        #region Fields

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the CreditsScreen class
        /// </summary>
        public CreditsScreen()
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
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
        }

        #endregion

        /// <summary>
        /// Responds to user input, accepting or cancelling the message box.
        /// </summary>
        /// <param name="input">The input state currently present in the system</param>
        public override void HandleInput(InputState input)
        {
            if (input.MenuCancel)
            {
                if (ScreenState == ScreenState.Active)
                {
                    ExitScreen();
                }
            }
        }

        #region Update and Draw

        /// <summary>
        /// Updates the background screen. Unlike most screens, this should not
        /// transition off even if it has been covered by another screen: it is
        /// supposed to be covered, after all! This overload forces the
        /// coveredByOtherScreen parameter to false in order to stop the base
        /// Update method wanting to transition off.
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

            Vector2 positionVector = new Vector2(100, 250);

            // XNA4.0 - Begin is all new
//            spriteBatch.Begin(SpriteBlendMode.AlphaBlend);
            spriteBatch.Begin();

            spriteBatch.Draw(
                             ScreenManager.BlankTexture,
                             new Rectangle(0, (int)(ScreenManager.GraphicsDevice.Viewport.Height * 0.25f), ScreenManager.GraphicsDevice.Viewport.Width, (int)(ScreenManager.GraphicsDevice.Viewport.Height * 0.75f)),
                             new Color(0, 0, 0, (byte)(TransitionAlpha / 2)));

            spriteBatch.DrawString(ScreenManager.Font, "Game Design by M. Brent", positionVector, new Color(240, 100, 180, fade));
            positionVector.Y += 50;

            spriteBatch.DrawString(ScreenManager.Font, "XNA Porting by Keith Henrickson (keith.henrickson@gmail.com)", positionVector, new Color(240, 100, 180, fade));
            positionVector.Y += 50;

            spriteBatch.DrawString(ScreenManager.Font, "Artwork by Brandon 'Binky' Weise", positionVector, new Color(240, 100, 180, fade));
            positionVector.Y += 50;

            spriteBatch.DrawString(ScreenManager.Font, "Controller artwork by Jeff Jenkins (sinnix@360prophecy.com)", positionVector, new Color(240, 100, 180, fade));
            positionVector.Y += 100;

            spriteBatch.DrawString(ScreenManager.Font, "For more information, see harmlesslion.com", positionVector, new Color(240, 100, 180, fade));
            positionVector.Y += 100;

            spriteBatch.End();
        }

        #endregion
    }
}
