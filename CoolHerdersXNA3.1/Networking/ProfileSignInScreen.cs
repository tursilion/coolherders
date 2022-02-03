//-----------------------------------------------------------------------------
// <copyright file="ProfileSignInScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// <para>
    /// In order to play a networked game, you must have a player profile signed in.
    /// If you want to play on Live, that has to be a Live profile. Rather than just
    /// failing with an error message, it is nice if we can automatically bring up the
    /// Guide screen when we detect that no suitable profiles are currently signed in,
    /// so the user can easily correct the problem. This screen checks the sign in
    /// state, and brings up the Guide user interface if there is a problem with it.
    /// It then raises an event as soon as a valid profile has been signed in.
    /// </para>
    /// <para>
    /// There are two scenarios for how this can work. If no good profile is signed in:
    ///     - MainMenuScreen activates the ProfileSignInScreen
    ///     - ProfileSignInScreen activates the Guide user interface
    ///     - User signs in a profile
    ///     - ProfileSignInScreen raises the ProfileSignedIn event
    ///     - This advances to the CreateOrFindSessionScreen
    /// </para>
    /// <para>
    /// Alternatively, there might already be a valid profile signed in. In this case:
    ///     - MainMenuScreen activates the ProfileSignInScreen
    ///     - ProfileSignInScreen notices everything is already good
    ///     - ProfileSignInScreen raises the ProfileSignedIn event
    ///     - This advances to the CreateOrFindSessionScreen
    /// </para>
    /// <para>
    /// In this second case, the ProfileSignInScreen is only active for a single
    /// Update, so the user just sees a transition directly from the MainMenuScreen
    /// to the CreateOrFindSessionScreen.
    /// </para>
    /// </summary>
    internal class ProfileSignInScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// The network session type we are creating
        /// </summary>
        private NetworkSessionType sessionType;

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
        public ProfileSignInScreen(NetworkSessionType sessionType)
        {
            this.sessionType = sessionType;

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
                    bool onlineOnly = (this.sessionType == NetworkSessionType.PlayerMatch);

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

            // If we want to play in a Live session, also make sure the profiles are
            // signed in to Live, and that they have the privilege for online gameplay.
            if (this.sessionType == NetworkSessionType.PlayerMatch)
            {
                for (int i = 0; i < Gamer.SignedInGamers.Count; i++)
                {
                    if (i >= NetworkSessionComponent.MaxLocalGamers)
                    {
                        break;
                    }

                    SignedInGamer gamer = Gamer.SignedInGamers[i];

                    if (!gamer.IsSignedInToLive)
                    {
                        return false;
                    }

                    if (!gamer.Privileges.AllowOnlineSessions)
                    {
                        return false;
                    }
                }
            }

            // Okeydokey, this looks good.
            return true;
        }

        #endregion
    }
}
