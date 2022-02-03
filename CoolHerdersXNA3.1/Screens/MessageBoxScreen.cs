//-----------------------------------------------------------------------------
// <copyright file="MessageBoxScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
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
    /// A popup message box screen, used to display "are you sure?"
    /// confirmation messages.
    /// </summary>
    public class MessageBoxScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// The message that we want displayed in this message box
        /// </summary>
        private string message;

        /// <summary>
        /// The description of the button A action
        /// </summary>
        private string buttonAText;

        /// <summary>
        /// The description of the button B action
        /// </summary>
        private string buttonBText;

        /// <summary>
        /// The gradient texture to use as the background
        /// </summary>
        private Texture2D gradientTexture;

        /// <summary>
        /// The texture for the A button
        /// </summary>
        private Texture2D buttonA;

        /// <summary>
        /// The texture for the B button
        /// </summary>
        private Texture2D buttonB;

        /// <summary>
        /// The font to use to draw the button text
        /// </summary>
        private SpriteFont buttonText;

        /// <summary>
        /// Should the usage text be shown
        /// </summary>
        private bool showUsageText;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MessageBoxScreen class
        /// </summary>
        /// <param name="message">The message that the message box should display</param>
        public MessageBoxScreen(string message)
            : this(message, true)
        {
        }

        /// <summary>
        /// Initializes a new instance of the MessageBoxScreen class
        /// </summary>
        /// <param name="message">The message to put in the message box</param>
        /// <param name="includeUsageText">Should the user be shown a default 'usage' message.</param>
        public MessageBoxScreen(string message, bool includeUsageText)
        {
            this.buttonAText = "Ok";
            this.buttonBText = "Cancel";
            
            this.message = message;

            this.showUsageText = includeUsageText;

            IsPopup = true;

            TransitionOnTime = TimeSpan.FromSeconds(0.2);
            TransitionOffTime = TimeSpan.FromSeconds(0.2);
        }

        #endregion

        #region Events

        /// <summary>
        /// An event that will fire when the message box is accepted
        /// </summary>
        public event EventHandler<EventArgs> Accepted;

        /// <summary>
        /// An event that will fire when the message box is canceled
        /// </summary>
        public event EventHandler<EventArgs> Canceled;

        #endregion

        /// <summary>
        /// Loads graphics content for this screen. This uses the shared ContentManager
        /// provided by the Game class, so the content will remain loaded forever.
        /// Whenever a subsequent MessageBoxScreen tries to load this same content,
        /// it will just get back another reference to the already loaded data.
        /// </summary>
        public override void LoadContent()
        {
            ContentManager content = ScreenManager.Game.Content;

            this.gradientTexture = content.Load<Texture2D>("MiscGraphics\\gradient");
            this.buttonA = content.Load<Texture2D>("Buttons - Small\\small_face_a");
            this.buttonB = content.Load<Texture2D>("Buttons - Small\\small_face_b");
            this.buttonText = content.Load<SpriteFont>("ArialButtonText");
        }

        #region Handle Input

        /// <summary>
        /// Responds to user input, accepting or cancelling the message box.
        /// </summary>
        /// <param name="input">The current input state of all controllers</param>
        public override void HandleInput(InputState input)
        {
            if (input.MenuSelect)
            {
                // Raise the accepted event, then exit the message box.
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                if (this.Accepted != null)
                {
                    this.Accepted(this, EventArgs.Empty);
                }

                ExitScreen();
            }
            else if (input.MenuCancel)
            {
                // Raise the cancelled event, then exit the message box.
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                if (this.Canceled != null)
                {
                    this.Canceled(this, EventArgs.Empty);
                }

                ExitScreen();
            }
        }

        #endregion

        #region Draw

        /// <summary>
        /// Draws the message box.
        /// </summary>
        /// <param name="gameTime">The current GameTime for this game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            // Darken down any other screens that were drawn beneath the popup.
            ScreenManager.FadeBackBufferToBlack(TransitionAlpha * 2 / 3);

            // Center the message text in the viewport.
            Viewport viewport = ScreenManager.GraphicsDevice.Viewport;
            Vector2 viewportSize = new Vector2(viewport.Width, viewport.Height);
            Vector2 textSize = font.MeasureString(this.message);
            Vector2 totalTextSize;
            Vector2 buttonATextSize;
            Vector2 buttonBTextSize;
            Vector2 buttonAPosition = Vector2.Zero;
            Vector2 buttonATextPosition = Vector2.Zero;
            Vector2 buttonBPosition = Vector2.Zero;
            Vector2 buttonBTextPosition = Vector2.Zero;
            Vector2 textPosition = (viewportSize - textSize) / 2;
            if (this.showUsageText)
            {
                buttonATextSize = this.buttonText.MeasureString(this.buttonAText);
                buttonBTextSize = this.buttonText.MeasureString(this.buttonBText);
                totalTextSize = new Vector2(textSize.X, textSize.Y + buttonATextSize.Y);
                buttonAPosition = new Vector2(textPosition.X, textPosition.Y + textSize.Y);
                buttonATextPosition = new Vector2(textPosition.X + this.buttonA.Width + (this.buttonA.Width * 0.01f), textPosition.Y + textSize.Y);
                buttonBPosition = new Vector2(textPosition.X + textSize.X - this.buttonB.Width, textPosition.Y + textSize.Y);
                buttonBTextPosition = new Vector2(buttonBPosition.X - buttonBTextSize.X, buttonBPosition.Y);
            }
            else
            {
                totalTextSize = new Vector2(textSize.X, textSize.Y);
            }

            // The background includes a border somewhat larger than the text itself.
            const int HorizontalPad = 32;
            const int VerticalPad = 16;

            Rectangle backgroundRectangle = new Rectangle(
                (int)textPosition.X - HorizontalPad,
                (int)textPosition.Y - VerticalPad,
                (int)totalTextSize.X + (HorizontalPad * 2),
                (int)totalTextSize.Y + (VerticalPad * 2));

            // Fade the popup alpha during transitions.
            Color color = new Color(255, 255, 255, TransitionAlpha);

            spriteBatch.Begin();

            // Draw the background rectangle.
            spriteBatch.Draw(this.gradientTexture, backgroundRectangle, color);

            if (this.showUsageText)
            {
                spriteBatch.Draw(this.buttonA, buttonAPosition, color);

                spriteBatch.Draw(this.buttonB, buttonBPosition, color);
            }

            // Draw the message box text.
            spriteBatch.DrawString(font, this.message, textPosition, color);

            if (this.showUsageText)
            {
                // Draw the button A string
                spriteBatch.DrawString(this.buttonText, this.buttonAText, buttonATextPosition, color);

                // Draw the button B string
                spriteBatch.DrawString(this.buttonText, this.buttonBText, buttonBTextPosition, color);
            }

            spriteBatch.End();
        }

        #endregion
    }
}
