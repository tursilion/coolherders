//-----------------------------------------------------------------------------
// <copyright file="NetworkSessionComponent.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using CoolHerders.Properties;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// Component in charge of owning and updating the current NetworkSession object.
    /// This is responsible for calling NetworkSession.Update at regular intervals,
    /// and also exposes the NetworkSession as a game service which can easily be
    /// looked up by any other code that needs to access it.
    /// </summary>
    internal class NetworkSessionComponent : GameComponent
    {
        #region Fields

        /// <summary>
        /// The maximum number of gamers which can participate
        /// </summary>
        public const int MaxGamers = 4;

        /// <summary>
        /// The maximum number of gamers which can join locally
        /// </summary>
        public const int MaxLocalGamers = 4;

        /// <summary>
        /// The screen manager that is responsible for drawing us
        /// </summary>
        private ScreenManager screenManager;

        /// <summary>
        /// The network session we are attached to
        /// </summary>
        private NetworkSession networkSession;

        /// <summary>
        /// An interface to the generic message display routine
        /// </summary>
        private IMessageDisplay messageDisplay;

        /// <summary>
        /// Set to true if the user needs to be notified on joins or leaves
        /// </summary>
        private bool notifyWhenPlayersJoinOrLeave;

        /// <summary>
        /// The message to display when the session ends.
        /// </summary>
        private string sessionEndMessage;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the NetworkSessionComponent class
        /// </summary>
        /// <param name="screenManager">The screen manager that this network session is part of</param>
        /// <param name="networkSession">The network session</param>
        public NetworkSessionComponent(
            ScreenManager screenManager,
            NetworkSession networkSession)
            : base(screenManager.Game)
        {
            this.screenManager = screenManager;
            this.networkSession = networkSession;

            // Hook up our session event handlers.
            this.networkSession.GamerJoined += this.GamerJoined;
            this.networkSession.GamerLeft += this.GamerLeft;
            this.networkSession.SessionEnded += this.NetworkSessionEnded;
        }

        /// <summary>
        /// Creates a new NetworkSessionComponent.
        /// </summary>
        /// <param name="screenManager">The screen manager for the screen we are viewing</param>
        /// <param name="networkSession">The network session we are working with</param>
        public static void Create(
            ScreenManager screenManager,
            NetworkSession networkSession)
        {
            Game game = screenManager.Game;

            // Register this network session as a service.
            game.Services.AddService(typeof(NetworkSession), networkSession);

            // Create a NetworkSessionComponent, and add it to the Game.
            game.Components.Add(new NetworkSessionComponent(screenManager, networkSession));
        }

        /// <summary>
        /// Public method called when the user wants to leave the network session.
        /// Displays a confirmation message box, then disposes the session, removes
        /// the NetworkSessionComponent, and returns them to the main menu screen.
        /// </summary>
        /// <param name="screenManager">The ScreenManger we are being drawn on</param>
        public static void LeaveSession(ScreenManager screenManager, bool forceExit)
        {
            // Search through Game.Components to find the NetworkSessionComponent.
            foreach (IGameComponent component in screenManager.Game.Components)
            {
                NetworkSessionComponent self = component as NetworkSessionComponent;

                if (self != null)
                {
                    if (!forceExit) {
                    // Display a message box to confirm the user really wants to leave.
                    string message;

                    if (self.networkSession.IsHost)
                    {
                        message = Resources.ConfirmEndSession;
                    }
                    else
                    {
                        message = Resources.ConfirmLeaveSession;
                    }

                    MessageBoxScreen confirmMessageBox = new MessageBoxScreen(message);

                    // Hook the messge box ok event to actually leave the session.
                    confirmMessageBox.Accepted += delegate
                    {
                        self.LeaveSession();
                    };

                    screenManager.AddScreen(confirmMessageBox);
                    }
                    else {
                        self.LeaveSession();
                    }

                    break;
                }
            }
        }

        /// <summary>
        /// Initializes the component.
        /// </summary>
        public override void Initialize()
        {
            base.Initialize();

            // Look up the IMessageDisplay service, which will
            // be used to report gamer join/leave notifications.
            this.messageDisplay = (IMessageDisplay)Game.Services.GetService(
                                                              typeof(IMessageDisplay));

            if (this.messageDisplay != null)
            {
                this.notifyWhenPlayersJoinOrLeave = true;
            }
        }

        #endregion

        #region Update

        /// <summary>
        /// Updates the network session.
        /// </summary>
        /// <param name="gameTime">The GameTime of the game</param>
        public override void Update(GameTime gameTime)
        {
            if (this.networkSession == null)
            {
                return;
            }

            try
            {
                this.networkSession.Update();

                // Has the session ended?
                if (this.networkSession.SessionState == NetworkSessionState.Ended)
                {
                    LeaveSession();
                }
            }
            catch (NetworkException exception)
            {
                // Handle any errors from the network session update.
                Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "NetworkSession.Update threw {0}: {1}", exception, exception.Message));

                this.sessionEndMessage = Resources.ErrorNetwork;

                LeaveSession();
            }
            catch (GamerServicesNotAvailableException exception)
            {
                Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "NetworkSession.Update threw {0}: {1}", exception, exception.Message));

                this.sessionEndMessage = Resources.GamerServicesUnavailableUpdate;
            }
        }

        #endregion

        #region Event Handlers

        /// <summary>
        /// Event handler called when a gamer joins the session.
        /// Displays a notification message.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">The joined event args</param>
        private void GamerJoined(object sender, GamerJoinedEventArgs e)
        {
            if (this.notifyWhenPlayersJoinOrLeave)
            {
                this.messageDisplay.ShowMessage(
                    Resources.MessageGamerJoined,
                    e.Gamer.Gamertag);
            }
        }

        /// <summary>
        /// Event handler called when a gamer leaves the session.
        /// Displays a notification message.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">The left event args</param>
        private void GamerLeft(object sender, GamerLeftEventArgs e)
        {
            if (this.notifyWhenPlayersJoinOrLeave)
            {
                this.messageDisplay.ShowMessage(
                    Resources.MessageGamerLeft,
                    e.Gamer.Gamertag);
            }
        }

        /// <summary>
        /// Event handler called when the network session ends.
        /// Stores the end reason, so this can later be displayed to the user.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">The session ended event args</param>
        private void NetworkSessionEnded(object sender, NetworkSessionEndedEventArgs e)
        {
            switch (e.EndReason)
            {
                case NetworkSessionEndReason.ClientSignedOut:
                    this.sessionEndMessage = null;
                    break;

                case NetworkSessionEndReason.HostEndedSession:
                    this.sessionEndMessage = Resources.ErrorHostEndedSession;
                    break;

                case NetworkSessionEndReason.RemovedByHost:
                    this.sessionEndMessage = Resources.ErrorRemovedByHost;
                    break;

                case NetworkSessionEndReason.Disconnected:
                default:
                    this.sessionEndMessage = Resources.ErrorDisconnected;
                    break;
            }

            this.notifyWhenPlayersJoinOrLeave = false;
        }

        #endregion

        #region Methods

        /// <summary>
        /// Internal method for leaving the network session. This disposes the 
        /// session, removes the NetworkSessionComponent, and returns the user
        /// to the main menu screen.
        /// </summary>
        private void LeaveSession()
        {
            // Remove the NetworkSessionComponent.
            Game.Components.Remove(this);

            // Remove the NetworkSession service.
            Game.Services.RemoveService(typeof(NetworkSession));

            // Dispose the NetworkSession.
            this.networkSession.Dispose();
            this.networkSession = null;

            // If we have a sessionEndMessage string explaining why the session has
            // ended (maybe this was a network disconnect, or perhaps the host kicked
            // us out?) create a message box to display this reason to the user.
            MessageBoxScreen messageBox;

            if (!string.IsNullOrEmpty(this.sessionEndMessage))
            {
                messageBox = new MessageBoxScreen(this.sessionEndMessage, false);
            }
            else
            {
                messageBox = null;
            }

            /* At this point we normally want to return the user all the way to the
            // main menu screen. But what if they just joined a session? In that case
            // they went through this flow of screens:
            //
            //  - MainMenuScreen
            //  - CreateOrFindSessionsScreen
            //  - JoinSessionScreen (if joining, skipped if creating a new session)
            //  - LobbyScreeen
            //
            // If we have these previous screens on the history stack, and the user
            // backs out of the LobbyScreen, the right thing is just to pop off the
            // LobbyScreen and JoinSessionScreen, returning them to the
            // CreateOrFindSessionsScreen (we cannot just back up to the
            // JoinSessionScreen, because it contains search results that will no
            // longer be valid). But if the user is in gameplay, or has been in
            // gameplay and then returned to the lobby, the screen stack will have
            // been emptied.
            //
            // To do the right thing in both cases, we scan through the screen history
            // stack looking for a CreateOrFindSessionScreen. If we find one, we pop
            // any subsequent screens so as to return back to it, while if we don't
            // find it, we just reset everything and go back to the main menu.
             */

            GameScreen[] screens = this.screenManager.GetScreens();

            // Look for the CreateOrFindSessionsScreen.
            for (int i = 0; i < screens.Length; i++)
            {
                if (screens[i] is CreateOrFindSessionScreen)
                {
                    // If we found one, pop everything since then to return back to it.
                    for (int j = i + 1; j < screens.Length; j++)
                    {
                        screens[j].ExitScreen();
                    }

                    // Display the why-did-the-session-end message box.
                    if (messageBox != null)
                    {
                        this.screenManager.AddScreen(messageBox);
                    }

                    return;
                }
            }

            // If we didn't find a CreateOrFindSessionsScreen, reset everything and
            // go back to the main menu. The why-did-the-session-end message box
            // will be displayed after the loading screen has completed.
            LoadingScreen.Load(this.screenManager, false, new BackgroundScreen(), new MainMenuScreen(), messageBox);
        }

        #endregion
    }
}
