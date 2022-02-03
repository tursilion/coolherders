//-----------------------------------------------------------------------------
// <copyright file="MainMenuScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using CoolHerders.Housekeeping;
    using CoolHerders.Networking;
    using CoolHerders.Properties;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// The main menu screen is the first thing displayed when the game starts up.
    /// </summary>
    internal class MainMenuScreen : MenuScreen
    {
        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MainMenuScreen class.
        /// </summary>
        public MainMenuScreen()
            : base(Resources.MainMenu)
        {
            if (GameInformation.Instance.StoryModeSequencer != null)
            {
                GameInformation.Instance.StoryModeSequencer.StopStory();
                GameInformation.Instance.StoryModeSequencer = null;
            }

            // Create our menu entries.
            MenuEntry storyModeGameMenuEntry = new FullModeMenuEntry("Story Mode");
            MenuEntry trialModeGameMenuEntry = new TrialModeMenuEntry("Trial Mode");
            MenuEntry multiplayerGameMenuEntry = new MenuEntry("Local Multiplayer");
            MenuEntry purchaseGameMenuEntry = new TrialModeMenuEntry("Buy game");
            MenuEntry systemLinkGameMenuEntry = new FullModeMenuEntry("System link");
            MenuEntry networkGameMenuEntry = new FullModeMenuEntry("XBox LIVE!");
            MenuEntry optionsMenuEntry = new MenuEntry("Options");
            MenuEntry creditsMenuEntry = new MenuEntry("Credits");
            MenuEntry exitMenuEntry = new MenuEntry("Exit");

            // Hook up menu event handlers.
            storyModeGameMenuEntry.Selected += this.StoryModeMenuEntry_Selected;
            trialModeGameMenuEntry.Selected += new EventHandler<EventArgs>(trialModeGameMenuEntry_Selected);
            multiplayerGameMenuEntry.Selected += this.MultiplayerMenuEntrySelected;
            purchaseGameMenuEntry.Selected += new EventHandler<EventArgs>(purchaseGameMenuEntry_Selected);
            systemLinkGameMenuEntry.Selected += this.SystemLinkMenuEntrySelected;
            networkGameMenuEntry.Selected += this.NetworkMenuEntrySelected;
            optionsMenuEntry.Selected += this.OptionsMenuEntrySelected;
            creditsMenuEntry.Selected += new EventHandler<EventArgs>(this.CreditsMenuEntry_Selected);
            exitMenuEntry.Selected += this.OnCancel;

            MenuEntries.Add(storyModeGameMenuEntry);
            MenuEntries.Add(trialModeGameMenuEntry);

            // Add entries to the menu.
            if (GameSettings.Instance.GamerServicesActive)
            {
                MenuEntries.Add(purchaseGameMenuEntry);
                MenuEntries.Add(multiplayerGameMenuEntry);
                MenuEntries.Add(networkGameMenuEntry);
                MenuEntries.Add(systemLinkGameMenuEntry);
            }

            MenuEntries.Add(optionsMenuEntry);
            MenuEntries.Add(creditsMenuEntry);
            MenuEntries.Add(exitMenuEntry);
        }

        void trialModeGameMenuEntry_Selected(object sender, EventArgs e)
        {
            SinglePlayerSignInScreen singleSignIn = new SinglePlayerSignInScreen();
            singleSignIn.ProfileSignedIn += new EventHandler<EventArgs>(trialSignIn_ProfileSignedIn);
            ScreenManager.AddScreen(singleSignIn);
        }

        /// <summary>
        /// Called when a valid single player has been signed in
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        void trialSignIn_ProfileSignedIn(object sender, EventArgs e)
        {
            ScreenManager.AddScreen(new SinglePlayerLoadScreen("EnglishTrial.txt"));
        }

        void purchaseGameMenuEntry_Selected(object sender, EventArgs e)
        {
            try
            {
                Guide.ShowMarketplace(GameInformation.Instance.MasterPlayerIndex);
            }
            catch (GamerPrivilegeException)
            {
                Guide.ShowSignIn(1, true);
            }
        }

        #endregion

        #region Handle Input

        /// <summary>
        /// When the user cancels the main menu, ask if they want to exit the sample.
        /// </summary>
        protected override void OnCancel()
        {
            const string Message = "Are you sure you want to exit Cool Herders?";

            MessageBoxScreen confirmExitMessageBox = new MessageBoxScreen(Message);

            confirmExitMessageBox.Accepted += this.ConfirmExitMessageBoxAccepted;

            ScreenManager.AddScreen(confirmExitMessageBox);
        }

        /// <summary>
        /// Event handler for when the Story Mode menu entry is selected
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void StoryModeMenuEntry_Selected(object sender, EventArgs e)
        {
            SinglePlayerSignInScreen singleSignIn = new SinglePlayerSignInScreen();
            singleSignIn.ProfileSignedIn += new EventHandler<EventArgs>(singleSignIn_ProfileSignedIn);
            ScreenManager.AddScreen(singleSignIn);
        }

        /// <summary>
        /// Called when a valid single player has been signed in
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        void singleSignIn_ProfileSignedIn(object sender, EventArgs e)
        {
            ScreenManager.AddScreen(new SinglePlayerLoadScreen("English.txt"));
        }

        /// <summary>
        /// Event handler for when the Credits menu entry is selected
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void CreditsMenuEntry_Selected(object sender, EventArgs e)
        {
            ScreenManager.AddScreen(new CreditsScreen());
        }

        /// <summary>
        /// Event handler for when the Network menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void NetworkMenuEntrySelected(object sender, EventArgs e)
        {
            this.CreateOrFindSession(NetworkSessionType.PlayerMatch);
        }

        /// <summary>
        /// Event handler for when the System Link menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void SystemLinkMenuEntrySelected(object sender, EventArgs e)
        {
            this.CreateOrFindSession(NetworkSessionType.SystemLink);
        }

        /// <summary>
        /// Event handler for when the Multiplayer menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void MultiplayerMenuEntrySelected(object sender, EventArgs e)
        {
            this.CreateOrFindSession(NetworkSessionType.Local);
        }

        /// <summary>
        /// Event handler for when the Options menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void OptionsMenuEntrySelected(object sender, EventArgs e)
        {
            ScreenManager.AddScreen(new OptionsMenuScreen());
        }

        /// <summary>
        /// Event handler for when the user selects ok on the "are you sure
        /// you want to exit" message box.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void ConfirmExitMessageBoxAccepted(object sender, EventArgs e)
        {
            ScreenManager.Game.Exit();
        }

        /// <summary>
        /// Helper method shared by the Live and System Link menu event handlers.
        /// </summary>
        /// <param name="sessionType">What type of network session is being created</param>
        private void CreateOrFindSession(NetworkSessionType sessionType)
        {
            // First, we need to make sure a suitable gamer profile is signed in.
            ProfileSignInScreen profileSignIn = new ProfileSignInScreen(sessionType);

            // Hook up an event so once the ProfileSignInScreen is happy,
            // it will activate the CreateOrFindSessionScreen.
            profileSignIn.ProfileSignedIn += delegate
            {
                if (NetworkSessionType.Local != sessionType)
                {
                    ScreenManager.AddScreen(new CreateOrFindSessionScreen(sessionType));
                }
                else
                {
                    try
                    {
                        NetworkSession networkSession = CreateLocalNetworkSession(NetworkSessionComponent.MaxLocalGamers, NetworkSessionComponent.MaxGamers);

                        // Go to the lobby screen.
                        ScreenManager.AddScreen(new LobbyScreen(networkSession));
                    }
                    catch (NetworkException exception)
                    {
                        ScreenManager.AddScreen(new NetworkErrorScreen(exception));
                    }
                    catch (GamerPrivilegeException exception)
                    {
                        ScreenManager.AddScreen(new NetworkErrorScreen(exception));
                    }
                }
            };

            // Activate the ProfileSignInScreen.
            ScreenManager.AddScreen(profileSignIn);
        }

        private NetworkSession CreateLocalNetworkSession(int maxLocalGamers, int maxGamers)
        {
            // Begin an synchronous create network session operation.
            NetworkSession networkSession = NetworkSession.Create(
                NetworkSessionType.Local,
                maxLocalGamers,
                maxGamers);

            // Create a component that will manage the session we just created.
            NetworkSessionComponent.Create(ScreenManager, networkSession);

            return networkSession;
        }

        #endregion
    }
}
