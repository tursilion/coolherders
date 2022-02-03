//-----------------------------------------------------------------------------
// <copyright file="NetworkBusyScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using System;
    using CoolHerders.Properties;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// When an asynchronous network operation (for instance searching for or joining a
    /// session) is in progress, we want to display some sort of busy indicator to let
    /// the user know the game hasn't just locked up. We also want to make sure they
    /// can't pick some other menu option before the current operation has finished.
    /// This screen takes care of both requirements in a single stroke. It monitors
    /// the IAsyncResult returned by an asynchronous network call, displaying a busy
    /// indicator for as long as the call is still in progress. When it notices the
    /// IAsyncResult has completed, it raises an event to let the game know it should
    /// proceed to the next step, after which the busy screen automatically goes away.
    /// Because this screen is on top of all others for as long as the asynchronous
    /// operation is in progress, it automatically takes over all user input,
    /// preventing any other menu entries being selected until the operation completes.
    /// </summary>
    internal class NetworkBusyScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// The result from the asynchronus network operation
        /// </summary>
        private IAsyncResult asyncResult;

        /// <summary>
        /// The gradient texture to show in the background
        /// </summary>
        private Texture2D gradientTexture;

        /// <summary>
        /// The texture of the rotating cat
        /// </summary>
        private Texture2D catTexture;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the NetworkBusyScreen class
        /// </summary>
        /// <param name="asyncResult">The place to store the result from the network operations</param>
        public NetworkBusyScreen(IAsyncResult asyncResult)
        {
            this.asyncResult = asyncResult;

            IsPopup = true;

            TransitionOnTime = TimeSpan.FromSeconds(0.1);
            TransitionOffTime = TimeSpan.FromSeconds(0.2);
        }

        #endregion

        #region Events

        /// <summary>
        /// An event that will fire when the network operation has completed
        /// </summary>
        public event EventHandler<OperationCompletedEventArgs> OperationCompleted;

        #endregion

        /// <summary>
        /// Loads graphics content for this screen. This uses the shared ContentManager
        /// provided by the Game class, so the content will remain loaded forever.
        /// Whenever a subsequent NetworkBusyScreen tries to load this same content,
        /// it will just get back another reference to the already loaded data.
        /// </summary>
        public override void LoadContent()
        {
            ContentManager content = ScreenManager.Game.Content;

            this.gradientTexture = content.Load<Texture2D>("MiscGraphics\\gradient");
            this.catTexture = content.Load<Texture2D>("MiscGraphics\\cat");
        }

        #region Update and Draw

        /// <summary>
        /// Updates the NetworkBusyScreen.
        /// </summary>
        /// <param name="gameTime">The current GameTime for this game</param>
        /// <param name="otherScreenHasFocus">Does some other screen have focus?</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by some other screen?</param>
        public override void Update(
            GameTime gameTime,
            bool otherScreenHasFocus,
            bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            // Has our asynchronous operation completed?
            if ((this.asyncResult != null) && this.asyncResult.IsCompleted)
            {
                // If so, raise the OperationCompleted event.
                if (this.OperationCompleted != null)
                {
                    this.OperationCompleted(this, new OperationCompletedEventArgs(this.asyncResult));
                }

                ExitScreen();

                this.asyncResult = null;
            }
        }

        /// <summary>
        /// Draws the NetworkBusyScreen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of this game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            string message = Resources.NetworkBusy;

            const int HorizontalPad = 32;
            const int VerticalPad = 16;

            // Center the message text in the viewport.
            Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
            Vector2 viewportSize = new Vector2(viewport.Width, viewport.Height);
            Vector2 textSize = font.MeasureString(message);

            // Add enough room to spin a cat.
            Vector2 catSize = new Vector2(this.catTexture.Width);

            textSize.X = Math.Max(textSize.X, catSize.X);
            textSize.Y += catSize.Y + VerticalPad;

            Vector2 textPosition = (viewportSize - textSize) / 2;

            // The background includes a border somewhat larger than the text itself.
            Rectangle backgroundRectangle = new Rectangle(
                (int)textPosition.X - HorizontalPad,
                (int)textPosition.Y - VerticalPad,
                (int)textSize.X + (HorizontalPad * 2),
                (int)textSize.Y + (VerticalPad * 2));

            // Fade the popup alpha during transitions.
            Color color = new Color(255, 255, 255, TransitionAlpha);

            spriteBatch.Begin();

            // Draw the background rectangle.
            spriteBatch.Draw(this.gradientTexture, backgroundRectangle, color);

            // Draw the message box text.
            spriteBatch.DrawString(font, message, textPosition, color);

            // Draw the spinning cat progress indicator.
            float catRotation = (float)gameTime.TotalGameTime.TotalSeconds * 3;

            Vector2 catPosition = new Vector2(textPosition.X + (textSize.X / 2), textPosition.Y + textSize.Y - (catSize.Y / 2));

            spriteBatch.Draw(
                this.catTexture,
                catPosition,
                null,
                color,
                catRotation,
                catSize / 2,
                1,
                SpriteEffects.None,
                0);

            spriteBatch.End();
        }

        #endregion
    }
}
