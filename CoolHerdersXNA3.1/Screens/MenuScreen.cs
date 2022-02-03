//-----------------------------------------------------------------------------
// <copyright file="MenuScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    #endregion

    /// <summary>
    /// Base class for screens that contain a menu of options. The user can
    /// move up and down to select an entry, or cancel to back out of the screen.
    /// </summary>
    internal abstract class MenuScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// The list of menu entries
        /// </summary>
        private List<MenuEntry> menuEntries = new List<MenuEntry>();

        /// <summary>
        /// The selected entry
        /// </summary>
        private int selectedEntry;

        /// <summary>
        /// The title of this menu
        /// </summary>
        private string menuTitle;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MenuScreen class.
        /// </summary>
        /// <param name="menuTitle">The title of this menu</param>
        public MenuScreen(string menuTitle)
        {
            this.menuTitle = menuTitle;

            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets the list of menu entries, so derived classes can add
        /// or change the menu contents.
        /// </summary>
        protected IList<MenuEntry> MenuEntries
        {
            get { return this.menuEntries; }
        }

        #endregion

        #region Handle Input

        /// <summary>
        /// Responds to user input, changing the selected entry and accepting
        /// or cancelling the menu.
        /// </summary>
        /// <param name="input">The input state of the system</param>
        public override void HandleInput(InputState input)
        {
            // Move to the previous menu entry?
            if (input.MenuUp)
            {
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                do
                {
                    this.selectedEntry--;
                    if (this.selectedEntry < 0)
                    {
                        this.selectedEntry = this.menuEntries.Count - 1;
                    }
                }
                while (!SelectionIsValid());
            }

            // Move to the next menu entry?
            if (input.MenuDown)
            {
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                do
                {
                    this.selectedEntry++;
                    if (this.selectedEntry >= this.menuEntries.Count)
                    {
                        this.selectedEntry = 0;
                    }
                }
                while (!SelectionIsValid());
            }

            // Accept or cancel the menu?
            if (input.MenuSelect)
            {
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                this.OnSelectEntry(this.selectedEntry);
            }
            else if (input.MenuCancel)
            {
                ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                this.OnCancel();
            }
        }

        private bool SelectionIsValid()
        {
            if ((Guide.IsTrialMode) && (this.menuEntries[this.selectedEntry] is FullModeMenuEntry))
            {
                return false;
            }
            if ((!Guide.IsTrialMode) && (this.menuEntries[this.selectedEntry] is TrialModeMenuEntry))
            {
                return false;
            }
            return true;
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Updates the menu.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        /// <param name="otherScreenHasFocus">Does some other screen have focus</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by another screen</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (!SelectionIsValid())
            {
                this.selectedEntry = 0;
                do
                {
                    this.selectedEntry++;
                }
                while (!SelectionIsValid());
            }

            // Update each nested MenuEntry object.
            for (int i = 0; i < this.menuEntries.Count; i++)
            {
                bool isSelected = IsActive && (i == this.selectedEntry);

                this.menuEntries[i].Update(this, isSelected, gameTime);
            }
        }

        /// <summary>
        /// Draws the menu.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the system</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            Viewport gameViewport = ScreenManager.GraphicsDevice.Viewport;

            Vector2 position = new Vector2(gameViewport.Width / 2, 280);

            // Make the menu slide into place during transitions, using a
            // power curve to make things look more interesting (this makes
            // the movement slow down as it nears the end).
            float transitionOffset = (float)Math.Pow(TransitionPosition, 2);

            if (ScreenState == ScreenState.TransitionOn)
            {
                position.Y += transitionOffset * 256;
            }
            else
            {
                position.Y += transitionOffset * 512;
            }

            spriteBatch.Begin();

            spriteBatch.Draw(
                             ScreenManager.BlankTexture,
                             new Rectangle(0, (int)(ScreenManager.GraphicsDevice.Viewport.Height * 0.25f), ScreenManager.GraphicsDevice.Viewport.Width, (int)(ScreenManager.GraphicsDevice.Viewport.Height * 0.75f)),
                             new Color(0, 0, 0, (byte)(TransitionAlpha / 2)));

            // Draw each menu entry in turn.
            for (int i = 0; i < this.menuEntries.Count; i++)
            {
                MenuEntry menuEntry = this.menuEntries[i];

                if ((Guide.IsTrialMode) && (menuEntry is FullModeMenuEntry))
                {
                    continue;
                }

                if ((!Guide.IsTrialMode) && (menuEntry is TrialModeMenuEntry))
                {
                    continue;
                }

                bool isSelected = IsActive && (i == this.selectedEntry);

                menuEntry.Draw(this, position, isSelected, gameTime);

                position.Y += menuEntry.GetHeight(this);
            }

            // Draw the menu title.
            Vector2 titlePosition = new Vector2(gameViewport.Width / 2, 220);
            Vector2 titleOrigin = font.MeasureString(this.menuTitle) / 2;
            Color titleColor = new Color(192, 192, 192, TransitionAlpha);
            float titleScale = 2.0f;

            titlePosition.Y -= transitionOffset * 100;

            spriteBatch.DrawString(
                font,
                this.menuTitle,
                titlePosition,
                titleColor,
                0,
                titleOrigin,
                titleScale,
                SpriteEffects.None,
                0);

            spriteBatch.End();
        }

        #endregion

        /// <summary>
        /// Handler for when the user has chosen a menu entry.
        /// </summary>
        /// <param name="entryIndex">The index of the entry which was selected</param>
        protected virtual void OnSelectEntry(int entryIndex)
        {
            this.menuEntries[selectedEntry].OnSelectEntry();
        }

        /// <summary>
        /// Handler for when the user has cancelled the menu.
        /// </summary>
        protected virtual void OnCancel()
        {
            ExitScreen();
        }

        /// <summary>
        /// Helper overload makes it easy to use OnCancel as a MenuEntry event handler.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        protected void OnCancel(object sender, EventArgs e)
        {
            this.OnCancel();
        }
    }
}
