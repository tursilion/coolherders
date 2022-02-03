//-----------------------------------------------------------------------------
// <copyright file="MessageDisplayComponent.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// Component implements the IMessageDisplay interface. This is used to show
    /// notification messages when interesting events occur, for instance when
    /// gamers join or leave the network session
    /// </summary>
    internal class MessageDisplayComponent : DrawableGameComponent, IMessageDisplay
    {
        #region Fields

        /// <summary>
        /// The time that this message will take to fade in
        /// </summary>
        private static readonly TimeSpan fadeInTime = TimeSpan.FromSeconds(0.25);

        /// <summary>
        /// The time that the message will be shown at full brightness
        /// </summary>
        private static readonly TimeSpan showTime = TimeSpan.FromSeconds(5);

        /// <summary>
        /// The time the message will take to fade out
        /// </summary>
        private static readonly TimeSpan fadeOutTime = TimeSpan.FromSeconds(0.5);

        /// <summary>
        /// A sprite batch which will be used to display the message box
        /// </summary>
        private SpriteBatch spriteBatch;

        /// <summary>
        /// A sprite font which will be used to display the message box
        /// </summary>
        private SpriteFont font;

        /// <summary>
        /// List of the currently visible notification messages.
        /// </summary>
        private List<NotificationMessage> messages = new List<NotificationMessage>();

        /// <summary>
        /// Coordinates threadsafe access to the message list.
        /// </summary>
        private object syncObject = new object();

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MessageDisplayComponent class.
        /// </summary>
        /// <param name="game">The game object responsible for supervising us</param>
        public MessageDisplayComponent(Game game)
            : base(game)
        {
            // Register ourselves to implement the IMessageDisplay service.
            game.Services.AddService(typeof(IMessageDisplay), this);
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Updates the message display component.
        /// </summary>
        /// <param name="gameTime">The game time object indicating the amount of time since the last update</param>
        public override void Update(GameTime gameTime)
        {
            lock (this.syncObject)
            {
                int index = 0;
                float targetPosition = 0;

                // Update each message in turn.
                while (index < this.messages.Count)
                {
                    NotificationMessage message = this.messages[index];

                    // Gradually slide the message toward its desired position.
                    float positionDelta = targetPosition - message.Position;

                    float velocity = (float)gameTime.ElapsedGameTime.TotalSeconds * 2;

                    message.Position += positionDelta * Math.Min(velocity, 1);

                    // Update the age of the message.
                    message.Age += gameTime.ElapsedGameTime;

                    if (message.Age < showTime + fadeOutTime)
                    {
                        // This message is still alive.
                        index++;

                        // Any subsequent messages should be positioned below
                        // this one, unless it has started to fade out.
                        if (message.Age < showTime)
                        {
                            targetPosition++;
                        }
                    }
                    else
                    {
                        // This message is old, and should be removed.
                        this.messages.RemoveAt(index);
                    }
                }
            }
        }

        /// <summary>
        /// Draws the message display component.
        /// </summary>
        /// <param name="gameTime">The current GameTime of this game</param>
        public override void Draw(GameTime gameTime)
        {
            lock (this.syncObject)
            {
                // Early out if there are no messages to display.
                if (this.messages.Count == 0)
                {
                    return;
                }

                Vector2 position = new Vector2(GraphicsDevice.Viewport.Width - 100, 0);

                this.spriteBatch.Begin();

                // Draw each message in turn.
                foreach (NotificationMessage message in this.messages)
                {
                    const float Scale = 0.75f;

                    // Compute the alpha of this message.
                    byte alpha = 255;

                    if (message.Age < fadeInTime)
                    {
                        // Fading in.
                        alpha = (byte)(255 * message.Age.TotalSeconds /
                                             fadeInTime.TotalSeconds);
                    }
                    else if (message.Age > showTime)
                    {
                        // Fading out.
                        TimeSpan fadeOut = showTime + fadeOutTime - message.Age;

                        alpha = (byte)(255 * fadeOut.TotalSeconds /
                                             fadeOutTime.TotalSeconds);
                    }

                    // Compute the message position.
                    position.Y = 80 + ((message.Position * this.font.LineSpacing) * Scale);

                    // Compute an origin value to right align each message.
                    Vector2 origin = this.font.MeasureString(message.Text);
                    origin.Y = 0;

                    // Draw the message text, with a drop shadow.
                    this.spriteBatch.DrawString(
                        this.font,
                        message.Text,
                        position + Vector2.One,
                        new Color(0, 0, 0, alpha),
                        0,
                        origin,
                        Scale,
                        SpriteEffects.None,
                        0);

                    this.spriteBatch.DrawString(
                        this.font,
                        message.Text,
                        position,
                        new Color(255, 255, 255, alpha),
                        0,
                        origin,
                        Scale,
                        SpriteEffects.None,
                        0);
                }

                this.spriteBatch.End();
            }
        }

        #endregion

        #region Implement IMessageDisplay

        /// <summary>
        /// Shows a new notification message.
        /// </summary>
        /// <param name="message">The text of the message that we want to show</param>
        /// <param name="parameters">Any parameters that need to be substituted into the message</param>
        public void ShowMessage(string message, params object[] parameters)
        {
            string formattedMessage = string.Format(CultureInfo.CurrentCulture, message, parameters);

            lock (this.syncObject)
            {
                float startPosition = this.messages.Count;

                this.messages.Add(new NotificationMessage(formattedMessage, startPosition));
            }
        }

        #endregion

        /// <summary>
        /// Load graphics content for the message display.
        /// </summary>
        protected override void LoadContent()
        {
            this.spriteBatch = new SpriteBatch(GraphicsDevice);

            this.font = Game.Content.Load<SpriteFont>("Arial");
        }

        #region Nested Types

        /// <summary>
        /// Helper class stores the position and text of a single notification message.
        /// </summary>
        internal class NotificationMessage
        {
            /// <summary>
            /// The text of the notification message
            /// </summary>
            public string Text;

            /// <summary>
            /// The position to draw the message at
            /// </summary>
            public float Position;

            /// <summary>
            /// The age of the message
            /// </summary>
            public TimeSpan Age;

            /// <summary>
            /// Initializes a new instance of the NotificationMessage class.
            /// </summary>
            /// <param name="text">The text of the notification message</param>
            /// <param name="position">The position of this message</param>
            public NotificationMessage(string text, float position)
            {
                this.Text = text;
                this.Position = position;
                this.Age = TimeSpan.Zero;
            }
        }

        #endregion
    }
}
