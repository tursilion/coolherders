//-----------------------------------------------------------------------------
// <copyright file="SinglePlayerSignInScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using CoolHerders.Housekeeping;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// </summary>
    internal class SinglePlayerSignInScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// Have we shown the guide
        /// </summary>
        private bool haveShownGuide;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the ProfileSignInScreen class
        /// </summary>
        /// <param name="sessionType">The network session type we eventually want to create</param>
        public SinglePlayerSignInScreen()
        {
            IsPopup = true;
        }

        #endregion

        #region Events

        /// <summary>
        /// An event that will fire when a profile signs in
        /// </summary>
        public event EventHandler<EventArgs> ProfileSignedIn;

        #endregion

        #region Update

        /// <summary>
        /// Updates the profile sign in screen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        /// <param name="otherScreenHasFocus">Does some other screen have focus</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by another screen</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (this.ValidProfileSignedIn())
            {
                // As soon as we detect a suitable profile is signed in,
                // we raise the profile signed in event, then go away.
                if (this.ProfileSignedIn != null)
                {
                    this.ProfileSignedIn(this, EventArgs.Empty);
                }

                ExitScreen();
            }
            else if (IsActive && !Guide.IsVisible)
            {
                if (!this.haveShownGuide)
                {
                    // No suitable profile is signed in, and we haven't already shown
                    // the Guide. Let's show it now, so they can sign in a profile.
                    bool onlineOnly = false;

                    Guide.ShowSignIn(NetworkSessionComponent.MaxLocalGamers, onlineOnly);

                    this.haveShownGuide = true;
                }
                else
                {
                    // Hmm. No suitable profile is signed in, but we already showed
                    // the Guide, and the Guide isn't still visible. There is only
                    // one thing that can explain this: they must have cancelled the
                    // Guide without signing in a profile. We'd better just exit,
                    // which will leave us on the same menu as before.
                    ExitScreen();
                }
            }
        }

        /// <summary>
        /// Helper checks whether a valid player profile is signed in.
        /// </summary>
        /// <returns>Returns true if all players signed in are allowed to play online</returns>
        private bool ValidProfileSignedIn()
        {
            // If there are no profiles signed in, that is never good.
            if (Gamer.SignedInGamers.Count == 0)
            {
                return false;
            }

            if (Gamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex] == null)
            {
                return false;
            }

            // Okeydokey, this looks good.
            return true;
        }

        #endregion
    }
}
