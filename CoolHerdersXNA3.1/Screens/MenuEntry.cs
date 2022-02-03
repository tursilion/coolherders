//-----------------------------------------------------------------------------
// <copyright file="MenuEntry.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// Helper class represents a single entry in a MenuScreen. By default this
    /// just draws the entry text string, but it can be customized to display menu
    /// entries in different ways. This also provides an event that will be raised
    /// when the menu entry is selected.
    /// </summary>
    internal class MenuEntry
    {
        #region Fields

        /// <summary>
        /// The text rendered for this entry.
        /// </summary>
        private string text;

        /// <summary>
        /// Tracks a fading selection effect on the entry.
        /// </summary>
        /// <remarks>
        /// The entries transition out of the selection effect when they are deselected.
        /// </remarks>
        private float selectionFade;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MenuEntry class
        /// </summary>
        /// <param name="text">The text of this menu entry</param>
        public MenuEntry(string text)
        {
            this.text = text;
        }

        #endregion

        #region Events

        /// <summary>
        /// Event raised when the menu entry is selected.
        /// </summary>
        public event EventHandler<EventArgs> Selected;

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets the text of this menu entry.
        /// </summary>
        public string Text
        {
            get { return this.text; }
            set { this.text = value; }
        }

        #endregion

        #region Update and Draw
        /// <summary>
        /// Updates the menu entry.
        /// </summary>
        /// <param name="screen">The menu screen that this entry is being drawn on</param>
        /// <param name="isSelected">Is this entry selected</param>
        /// <param name="gameTime">The current GameTime of this game</param>
        public virtual void Update(MenuScreen screen, bool isSelected, GameTime gameTime)
        {
            // When the menu selection changes, entries gradually fade between
            // their selected and deselected appearance, rather than instantly
            // popping to the new state.
            float fadeSpeed = (float)gameTime.ElapsedGameTime.TotalSeconds * 4;

            if (isSelected)
            {
                this.selectionFade = Math.Min(this.selectionFade + fadeSpeed, 1);
            }
            else
            {
                this.selectionFade = Math.Max(this.selectionFade - fadeSpeed, 0);
            }
        }

        /// <summary>
        /// Draws the menu entry. This can be overridden to customize the appearance.
        /// </summary>
        /// <param name="screen">The menu screen that this entry is being drawn on</param>
        /// <param name="position">The position this entry is to be drawn at</param>
        /// <param name="isSelected">Is this option selected</param>
        /// <param name="gameTime">The current GameTime of the game</param>
        public virtual void Draw(MenuScreen screen, Vector2 position, bool isSelected, GameTime gameTime)
        {
            // Draw the selected entry in yellow, otherwise white.
            Color color = isSelected ? Color.Yellow : Color.White;
            
            // Pulsate the size of the selected menu entry.
            double time = gameTime.TotalGameTime.TotalSeconds;
            
            float pulsate = (float)Math.Sin(time * 6) + 1;

            float scale = 1 + ((pulsate * 0.05f) * this.selectionFade);

            // Modify the alpha to fade text out during transitions.
            color = new Color(color.R, color.G, color.B, screen.TransitionAlpha);

            // Draw text, centered on the middle of each line.
            ScreenManager screenManager = screen.ScreenManager;
            SpriteBatch spriteBatch = screenManager.SpriteBatch;
            SpriteFont font = screenManager.Font;

            Vector2 origin = new Vector2(font.MeasureString(this.text).X / 2, font.LineSpacing / 2);

            spriteBatch.DrawString(
                font,
                this.text,
                position,
                color,
                0,
                origin,
                scale,
                SpriteEffects.None,
                0);
        }

        /// <summary>
        /// Queries how much space this menu entry requires.
        /// </summary>
        /// <param name="screen">The screen that this entry is being drawn on</param>
        /// <returns>The height in pixels of the space needed for this line</returns>
        public virtual int GetHeight(MenuScreen screen)
        {
            return screen.ScreenManager.Font.LineSpacing;
        }

        #endregion

        /// <summary>
        /// Method for raising the Selected event.
        /// </summary>
        protected internal virtual void OnSelectEntry()
        {
            if (this.Selected != null)
            {
                this.Selected(this, EventArgs.Empty);
            }
        }
    }
}
