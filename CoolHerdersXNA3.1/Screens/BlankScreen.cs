//-----------------------------------------------------------------------------
// <copyright file="BlankScreen.cs" company="HarmlessLion">
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
    /// Draws a blank screen that automatically fades out.  Useful for pauses in the story mode.
    /// </summary>
    internal class BlankScreen : GameScreen
    {
        #region Fields

        private TimeSpan remainingTime;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the TitleScreen class
        /// </summary>
        public BlankScreen()
        {
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
            remainingTime = TimeSpan.FromSeconds(1);
        }
        #endregion

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
            remainingTime -= gameTime.ElapsedGameTime;
            if ((this.IsActive) && (remainingTime.Ticks <= 0))
            {
                GameInformation.Instance.StoryModeSequencer.RunNextSequence(false, false);
            }
        }

        /// <summary>
        /// Draws the background screen.
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            ScreenManager.FadeBackBufferToBlack(TransitionAlpha);
        }

        #endregion
    }
}
