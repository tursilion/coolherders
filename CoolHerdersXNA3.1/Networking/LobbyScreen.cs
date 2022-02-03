//-----------------------------------------------------------------------------
// <copyright file="LobbyScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using System.Collections.Generic;
    using CoolHerders.Housekeeping;
    using CoolHerders.LevelDescriptions;
    using CoolHerders.Properties;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;
    #endregion

    // Network packet IDs
    // 0x80 - Greeting from server (id, seat number) host -> client
    // 0x81 - Greeting from client (color tints) client -> host
    // 0x82 - Player information host -> client
    // 0x83 - Increment avatar client -> host
    // 0x84 - Change map set host -> client

    /// <summary>
    /// The lobby screen provides a place for gamers to congregate before starting
    /// the actual gameplay. It displays a list of all the gamers in the session,
    /// and indicates which ones are currently talking. Each gamer can press a button
    /// to mark themselves as ready: gameplay will begin after everyone has done this.
    /// </summary>
    internal class LobbyScreen : GameScreen
    {
        #region Fields

        /// <summary>
        /// The colors to tint each player's background
        /// </summary>
        private static readonly Color[] playerColors =
        {
            Color.LightBlue,
            Color.LightGreen,
            Color.LightYellow,
            Color.LightPink,
        };

        /// <summary>
        /// A mapping table of system colors (selected in Game Defaults), to texture page colors
        /// </summary>
        private static readonly Nullable<Color>[] characterColors = 
        {
            Color.Gray,
            Color.Brown,
            Color.Red,
            Color.Pink,
            Color.Orange,
            Color.Yellow,
            null,
            Color.Green,
            null,
            null,
            null,
            Color.Blue,
            null,
            Color.Purple,
            null,
            null,
        };

        /// <summary>
        /// The origins for each player's info window
        /// </summary>
        private static readonly Vector2[] playerInfoCards =
        {
          new Vector2(150, 0),
          new Vector2(650, 0),
          new Vector2(150, 300),
          new Vector2(650, 300)
        };

        /// <summary>
        /// The names of the characters available in network mode
        /// </summary>
        private static readonly string[] characterNames = 
        {
            "Angel", "Backup", "Candy", "Classic", "Demon", "Hades", "Iskur", "NH5", "Sheep", "SheepAfro", "Thalia", "Trey", "Zeus", "ZeusAfro", "Zombie"
        };

        /// <summary>
        /// The level that is currently selected
        /// </summary>
        private byte currentLevelNumber;
        
        /// <summary>
        /// The network session for which we are in a lobby
        /// </summary>
        private NetworkSession networkSession;

        /// <summary>
        /// The texture to show when a player is ready
        /// </summary>
        private Texture2D isReadyTexture;

        /// <summary>
        /// The texture to show when a player has voice capability
        /// </summary>
        private Texture2D hasVoiceTexture;

        /// <summary>
        /// The texture to show when a player is talking
        /// </summary>
        private Texture2D isTalkingTexture;

        /// <summary>
        /// The texture to show when a player is muted
        /// </summary>
        private Texture2D voiceMutedTexture;

        /// <summary>
        /// The texture to show behind each player's info panel
        /// </summary>
        private Texture2D playerBackgroundTexture;

        /// <summary>
        /// The character image for all selectable characters
        /// </summary>
        private Texture2D characterSamples;

        /// <summary>
        /// The packet writer for use in the lobby
        /// </summary>
        private PacketWriter lobbyPacketWriter = new PacketWriter();

        /// <summary>
        /// The packet reader for use in the lobby
        /// </summary>
        private PacketReader lobbyPacketReader = new PacketReader();

        /// <summary>
        /// The texture for the X button
        /// </summary>
        private Texture2D buttonX;

        /// <summary>
        /// The texture for the Y button
        /// </summary>
        private Texture2D buttonY;

        /// <summary>
        /// Which of the four seats are available.  Only used by the game host.
        /// </summary>
        private bool[] availableSeats = new bool[4];

        /// <summary>
        /// The choice of character for this seat
        /// </summary>
        private byte[] characterChoice = new byte[4];

        /// <summary>
        /// A list of gamers which are signed in, but have not yet been added to the current network session
        /// </summary>
        private List<SignedInGamer> pendingGamers = new List<SignedInGamer>();

        /// <summary>
        /// The number of controllers found during the last update
        /// </summary>
        private int numberOfAttachedControllers;

        /// <summary>
        /// A source of random numbers
        /// </summary>
        private Random randomSource = new Random();

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the LobbyScreen class
        /// </summary>
        /// <param name="networkSession">The network session for which we need to create a lobby</param>
        public LobbyScreen(NetworkSession networkSession)
        {
            this.networkSession = networkSession;

            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);

            this.availableSeats[0] = true;
            this.availableSeats[1] = true;
            this.availableSeats[2] = true;
            this.availableSeats[3] = true;

            this.characterChoice[0] = 10;
            this.characterChoice[1] = 10;
            this.characterChoice[2] = 10;
            this.characterChoice[3] = 10;
        }

        /// <summary>
        /// Loads graphics content used by the lobby screen.
        /// </summary>
        public override void LoadContent()
        {
            ContentManager content = ScreenManager.Game.Content;

            this.isReadyTexture = content.Load<Texture2D>("MiscGraphics\\chat_ready");
            this.hasVoiceTexture = content.Load<Texture2D>("MiscGraphics\\chat_able");
            this.isTalkingTexture = content.Load<Texture2D>("MiscGraphics\\chat_talking");
            this.voiceMutedTexture = content.Load<Texture2D>("MiscGraphics\\chat_mute");
            this.playerBackgroundTexture = content.Load<Texture2D>("MiscGraphics\\gradient2");
            this.characterSamples = content.Load<Texture2D>("Characters\\characters");

            this.buttonX = content.Load<Texture2D>("Buttons - Small\\small_face_x");
            this.buttonY = content.Load<Texture2D>("Buttons - Small\\small_face_y");

            this.networkSession.GamerJoined += new EventHandler<GamerJoinedEventArgs>(this.NetworkSession_GamerJoined);
            this.networkSession.GamerLeft += new EventHandler<GamerLeftEventArgs>(this.NetworkSession_GamerLeft);
            SignedInGamer.SignedIn += new EventHandler<SignedInEventArgs>(this.SignedInGamer_SignedIn);
            SignedInGamer.SignedOut += new EventHandler<SignedOutEventArgs>(this.SignedInGamer_SignedOut);
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
            SignedInGamer.SignedIn -= new EventHandler<SignedInEventArgs>(this.SignedInGamer_SignedIn);
            SignedInGamer.SignedOut -= new EventHandler<SignedOutEventArgs>(this.SignedInGamer_SignedOut);
        }

        #endregion

        #region Update

        /// <summary>
        /// Updates the lobby screen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of this game</param>
        /// <param name="otherScreenHasFocus">Does some other screen have focus</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by another screen</param>
        public override void Update(
            GameTime gameTime,
            bool otherScreenHasFocus,
            bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (!IsExiting)
            {
                if (this.networkSession.SessionState == NetworkSessionState.Playing)
                {
                    List<PlayerInformation> enemyPlayers = new List<PlayerInformation>();

                    if (this.networkSession.IsHost)
                    {
                        // Temporary Hack for Computer Player test
                        for (byte seatCounter = 0; seatCounter < GameSettings.Instance.MinimumNumberOfHerders; seatCounter++)
                        {
                            if (this.availableSeats[seatCounter])
                            {
                                PlayerInformation enemyPlayer = new PlayerInformation();
                                enemyPlayer.CharacterClass = "Iskur";
                                enemyPlayer.PlayerActive = true;
                                enemyPlayer.SeatNumber = seatCounter;
                                enemyPlayer.PlayerColorIndex = seatCounter;
                                enemyPlayer.SheepCurrentRound = 0;
                                enemyPlayer.SheepTotal = 0;
                                enemyPlayer.SkillLevel = 10;
                                enemyPlayers.Add(enemyPlayer);
                            }
                        }
                    }

                    GameInformation.Instance.WorldScreen = LevelDescription.LoadLevelFromCode((BuiltInLevel)this.currentLevelNumber);
                    GameInformation.Instance.CurrentStageNumber = 0;
                    GameInformation.Instance.EnemyPlayers = enemyPlayers;

                    // Check if we should leave the lobby and begin gameplay.
                    LoadingScreen.Load(
                        ScreenManager,
                        true,
                        new MazeScreen(this.networkSession));
                }
                else if (this.networkSession.IsHost && this.networkSession.IsEveryoneReady && (this.networkSession.AllGamers.Count > 1))
                {
                    // The host checks whether everyone has marked themselves
                    // as ready, and starts the game in response.
                    this.networkSession.StartGame();
                }
            }
        }

        /// <summary>
        /// Handles user input for all the local gamers in the session. Unlike most
        /// screens, which use the InputState class to combine input data from all
        /// gamepads, the lobby needs to individually mark specific players as ready,
        /// so it loops over all the local gamers and reads their inputs individually.
        /// </summary>
        /// <param name="input">The current input state</param>
        public override void HandleInput(InputState input)
        {
            HandlePendingLocalGamers(input);

            foreach (LocalNetworkGamer gamer in this.networkSession.LocalGamers)
            {
                PlayerIndex playerIndex = gamer.SignedInGamer.PlayerIndex;

                if (input.IsMenuSelect(playerIndex))
                {
                    this.HandleMenuSelect(gamer);
                }
                else if (input.IsMenuCancel(playerIndex))
                {
                    this.HandleMenuCancel(gamer);
                }
                else if (input.IsNewButtonPress(Buttons.X, playerIndex) || input.IsNewKeyPress(Keys.X, playerIndex))
                {
                    SelectNextAvatar(gamer);
                }
                else if (input.IsNewButtonPress(Buttons.Y, playerIndex) || input.IsNewKeyPress(Keys.Y, playerIndex))
                {
                    SelectNextMapSet(gamer);
                }

                while (gamer.IsDataAvailable)
                {
                    NetworkGamer sendingPlayer;

                    gamer.ReceiveData(this.lobbyPacketReader, out sendingPlayer);
                    while (this.lobbyPacketReader.Position < this.lobbyPacketReader.Length)
                    {
                        int networkCommand = this.lobbyPacketReader.ReadByte();
                        if (networkCommand < 0x80)
                        {
                            // This is a stray packet from the previous game.  Ignore
                            break;
                        }
                        switch (networkCommand)
                        {
                            case 0x80:
                                {
                                    // Greeting from host, with our assigned seat number.
                                    byte seatNumber = this.lobbyPacketReader.ReadByte();

                                    // Reply to the host, with our color requests 
                                    this.lobbyPacketWriter.Write((byte)0x81);
                                    this.lobbyPacketWriter.Write(seatNumber);
                                    this.lobbyPacketWriter.Write(DecodeColor(gamer.SignedInGamer.GameDefaults.PrimaryColor));
                                    this.lobbyPacketWriter.Write(DecodeColor(gamer.SignedInGamer.GameDefaults.SecondaryColor));
                                    gamer.SendData(this.lobbyPacketWriter, SendDataOptions.ReliableInOrder, sendingPlayer);
                                    break;
                                }

                            case 0x81:
                                {
                                    // Reply from client, with color requests
                                    byte seatNumber = this.lobbyPacketReader.ReadByte();
                                    byte primaryColor = this.lobbyPacketReader.ReadByte();
                                    byte secondaryColor = this.lobbyPacketReader.ReadByte();
                                    byte characterNumber = 10;

                                    PlayerInformation playerInfo = CreatePlayerInformation(seatNumber, characterNumber);
                                    sendingPlayer.Tag = playerInfo;
                                    playerInfo.PlayerColorIndex = SelectPlayerColorIndex(primaryColor, secondaryColor, seatNumber);

                                    object gamerId = sendingPlayer.Id;
                                    sendingPlayer.BeginGetProfile(this.EndGetProfile, gamerId);

                                    UpdateAllPlayersInSession(gamer);
                                    break;
                                }

                            case 0x82:
                                {
                                    byte gamerId = this.lobbyPacketReader.ReadByte();
                                    byte seatNumber = this.lobbyPacketReader.ReadByte();
                                    byte characterNumber = this.lobbyPacketReader.ReadByte();
                                    byte playerColorIndex = this.lobbyPacketReader.ReadByte();

                                    NetworkGamer updatedGamer = this.networkSession.FindGamerById(gamerId);
                                    if (updatedGamer != null)
                                    {
                                        PlayerInformation playerInfo = CreatePlayerInformation(seatNumber, characterNumber);
                                        playerInfo.PlayerColorIndex = playerColorIndex;

                                        if (updatedGamer.IsLocal)
                                        {
                                            LocalNetworkGamer localNetGamer = (LocalNetworkGamer)updatedGamer;
                                            playerInfo.SignedInGamer = localNetGamer.SignedInGamer;
                                        }

                                        updatedGamer.Tag = playerInfo;
                                        updatedGamer.BeginGetProfile(this.EndGetProfile, updatedGamer.Id);

                                    }
                                    break;
                                }

                            case 0x83:
                                {
                                    PlayerInformation playerInfo = (PlayerInformation)sendingPlayer.Tag;
                                    this.characterChoice[playerInfo.SeatNumber] = (byte)((this.characterChoice[playerInfo.SeatNumber] + 1) % 15);
                                    playerInfo.CharacterClass = characterNames[this.characterChoice[playerInfo.SeatNumber]];

                                    UpdateAllPlayersInSession(gamer);
                                    break;
                                }

                            case 0x84:
                                byte newLevelNumber = this.lobbyPacketReader.ReadByte();
                                this.currentLevelNumber = newLevelNumber;
                                if (!GameInformation.Instance.AvailableLevels[this.currentLevelNumber].IsLevelUnlocked)
                                {
                                    gamer.IsReady = false;
                                }
                                break;
                        }
                    }
                }
            }
        }

        private PlayerInformation CreatePlayerInformation( byte seatNumber, byte characterNumber)
        {
            this.characterChoice[seatNumber] = characterNumber;

            PlayerInformation playerInfo = new PlayerInformation();
            playerInfo.PlayerActive = true;
            playerInfo.SeatNumber = seatNumber;
            playerInfo.SheepCurrentRound = 0;
            playerInfo.SheepTotal = 0;
            playerInfo.SkillLevel = 0;
            playerInfo.CharacterClass = characterNames[this.characterChoice[seatNumber]];

            return playerInfo;
        }

        private void UpdateAllPlayersInSession(LocalNetworkGamer gamer)
        {
            foreach (NetworkGamer netGamer in this.networkSession.AllGamers)
            {
                this.lobbyPacketWriter.Write((byte)0x82);
                this.lobbyPacketWriter.Write(netGamer.Id);
                this.lobbyPacketWriter.Write(((PlayerInformation)netGamer.Tag).SeatNumber);
                this.lobbyPacketWriter.Write(this.characterChoice[((PlayerInformation)netGamer.Tag).SeatNumber]);
                this.lobbyPacketWriter.Write(((PlayerInformation)netGamer.Tag).PlayerColorIndex);
            }
            gamer.SendData(this.lobbyPacketWriter, SendDataOptions.ReliableInOrder);

        }

        private void SelectNextMapSet(LocalNetworkGamer gamer)
        {
            if (gamer.IsHost)
            {
                this.currentLevelNumber++;
                if (this.currentLevelNumber >= GameInformation.Instance.AvailableLevels.Count)
                {
                    this.currentLevelNumber = 0;
                }
                if (!GameInformation.Instance.AvailableLevels[this.currentLevelNumber].IsLevelUnlocked)
                {
                    this.networkSession.ResetReady();
                }
                this.lobbyPacketWriter.Write((byte)0x84);
                this.lobbyPacketWriter.Write(this.currentLevelNumber);
                gamer.SendData(this.lobbyPacketWriter, SendDataOptions.ReliableInOrder);
            }
        }

        private void SelectNextAvatar(LocalNetworkGamer gamer)
        {
            if (gamer.Tag != null)
            {
                this.lobbyPacketWriter.Write((byte)0x83);
                gamer.SendData(this.lobbyPacketWriter, SendDataOptions.ReliableInOrder, this.networkSession.Host);
            }
        }

        /// <summary>
        /// Handles local gamers that are not yet part of this game session
        /// This is things like:
        /// Noting which controllers are not plugged in (we support 4)
        /// Noting which controllers are plugged in with no one signed in
        /// Noting which controllers have a signed in gamer who is not registered to play
        /// </summary>
        /// <param name="input">The current input state</param>
        private void HandlePendingLocalGamers(InputState input)
        {
            // First, zero out the number of attached controllers
            this.numberOfAttachedControllers = 0;

            // Go through the player indexes looking for any attached controllers
            for (PlayerIndex index = PlayerIndex.One; index < PlayerIndex.Four; index++)
            {
                GamePadState state = input.CurrentGamePadStates[(int)index];
                if (state.IsConnected)
                {
                    this.numberOfAttachedControllers++;
                }

                // We found a controller that does not have a signed in gamer
                if (null == SignedInGamer.SignedInGamers[index])
                {
                    if (input.IsMenuSelect(index))
                    {
                        if (!Guide.IsVisible)
                        {
                            if (this.networkSession.SessionType == NetworkSessionType.PlayerMatch)
                            {
                                Guide.ShowSignIn(1, true);
                            }
                            else
                            {
                                Guide.ShowSignIn(1, false);
                            }
                        }
                    }
                }
            }

            // And finally, if someone is signed in, go ahead and add them to the game.
            foreach (SignedInGamer gamer in this.pendingGamers)
            {
                if (input.IsMenuSelect(gamer.PlayerIndex))
                {
                    this.networkSession.AddLocalGamer(gamer);
                }
            }

        }

        #endregion

        #region Draw

        /// <summary>
        /// Draws the lobby screen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of this game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            // Make the lobby slide into place during transitions.
            float transitionOffset = (float)Math.Pow(TransitionPosition, 2);

            spriteBatch.Begin();

            int pendingGamerCount = this.pendingGamers.Count;
            int availableControllerPorts = this.numberOfAttachedControllers;

            for (byte seatCounter = 0; seatCounter < 4; seatCounter++)
            {
                bool gamerFound = false;
                foreach (NetworkGamer gamer in this.networkSession.AllGamers)
                {
                    PlayerInformation playerInfo = ((PlayerInformation)gamer.Tag);
                    if (playerInfo == null) {
                        continue;
                    }
                    int playerSeatNumber = playerInfo.SeatNumber;
                    if (playerSeatNumber == seatCounter)
                    {
                        if (gamer.IsLocal)
                        {
                            availableControllerPorts--;
                        }

                        this.DrawGamer(gamer);
                        gamerFound = true;
                        break;
                    }
                }

                if (!gamerFound)
                {
                    if (pendingGamerCount > 0)
                    {
                        this.DrawPendingGamer(seatCounter);
                        pendingGamerCount--;
                        availableControllerPorts--;
                    }
                    else
                    {
                        if (availableControllerPorts > 0)
                        {
                            this.DrawOpenGamer(seatCounter);
                            availableControllerPorts--;
                        }
                        else
                        {
                            this.DrawNeedController(seatCounter);
                        }
                    }
                }
            }

            // Draw the screen title.
            string levelName = string.Format("{0}: {1}", "Level name", GameInformation.Instance.AvailableLevels[this.currentLevelNumber].DisplayName);

            Vector2 levelPosition = new Vector2(640, 630);
            Vector2 levelOrigin = font.MeasureString(levelName) / 2;
            Color levelColor;

            if (GameInformation.Instance.AvailableLevels[this.currentLevelNumber].IsLevelUnlocked)
            {
                levelColor = new Color(220, 220, 220, TransitionAlpha);
            }
            else
            {
                // XNA4.0
                //levelColor = new Color(Color.SlateGray, TransitionAlpha);
                levelColor = new Color(Color.SlateGray.R, Color.SlateGray.G, Color.SlateGray.B, TransitionAlpha);
            }

            float levelScale = 1.25f;

            spriteBatch.DrawString(font, levelName, levelPosition, levelColor, 0, levelOrigin, levelScale, SpriteEffects.None, 0);

            string instructionsText = "    to select character.       to select level (host only)";

            Vector2 instructionsPosition = new Vector2(640, 680);
            Vector2 instructionsOrigin = font.MeasureString(instructionsText) / 2;
            Color instructionsColor = new Color(220, 220, 220, TransitionAlpha);
            float instructionsScale = 1.25f;

            spriteBatch.DrawString(font, instructionsText, instructionsPosition, instructionsColor, 0, instructionsOrigin, instructionsScale, SpriteEffects.None, 0);

            spriteBatch.Draw(this.buttonX, new Vector2(180, 650), Color.White);
            spriteBatch.Draw(this.buttonY, new Vector2(585, 650), Color.White);

            spriteBatch.End();
        }

        /// <summary>
        /// Helper draws the gamertag and status icons for a single NetworkGamer.
        /// </summary>
        /// <param name="gamer">The gamer to draw status for</param>
        private void DrawGamer(NetworkGamer gamer)
        {
            if (gamer.Tag == null)
            {
                return;
            }

            int playerSeatNumber = ((PlayerInformation)gamer.Tag).SeatNumber;

            Vector2 position = playerInfoCards[playerSeatNumber];
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            Vector2 iconWidth = new Vector2(34, 0);
            Vector2 iconOffset = new Vector2(0, 12);

            Vector2 characterPosition = new Vector2(200, 50) + position;
            Rectangle characterRectangle = new Rectangle(this.characterChoice[playerSeatNumber] * 48, 0, 48, 48);

            Vector2 gamerPicturePosition = new Vector2(68, 50) + position;

            Vector2 iconPosition = position + iconOffset;
            Rectangle backgroundBox = new Rectangle((int)position.X, (int)position.Y, 500, 300);

            spriteBatch.Draw(this.playerBackgroundTexture, backgroundBox, playerColors[playerSeatNumber]);

            spriteBatch.Draw(this.characterSamples, characterPosition, characterRectangle, Color.White, 0.0f, Vector2.Zero, 1.5f, SpriteEffects.None, 0.0f);

            GamerProfile gamerProfile = ((PlayerInformation)gamer.Tag).GamerProfile;
            if (gamerProfile != null)
            {
                // XNA4.0 - GamerPicture removed - generate dynamically?
                Texture2D gamerPic = Texture2D.FromStream(spriteBatch.GraphicsDevice, gamerProfile.GetGamerPicture());
                if (null != gamerPic)
                {
                    spriteBatch.Draw(gamerPic, gamerPicturePosition, Color.White);
                }
            }

            // Draw the "is ready" icon.
            if (gamer.IsReady)
            {
                spriteBatch.Draw(this.isReadyTexture, iconPosition, this.FadeAlphaDuringTransition(Color.Lime));
            }

            iconPosition += iconWidth;

            // Draw the "is muted", "is talking", or "has voice" icon.
            if (gamer.IsMutedByLocalUser)
            {
                spriteBatch.Draw(this.voiceMutedTexture, iconPosition, this.FadeAlphaDuringTransition(Color.Red));
            }
            else if (gamer.IsTalking)
            {
                spriteBatch.Draw(this.isTalkingTexture, iconPosition, this.FadeAlphaDuringTransition(Color.Yellow));
            }
            else if (gamer.HasVoice)
            {
                spriteBatch.Draw(this.hasVoiceTexture, iconPosition, this.FadeAlphaDuringTransition(Color.White));
            }

            // Draw the gamertag, normally in white, but yellow for local players.
            string text = gamer.Gamertag;

            if (gamer.IsHost)
            {
                text += Resources.HostSuffix;
            }

            Color color = gamer.IsLocal ? Color.Yellow : Color.White;

            spriteBatch.DrawString(font, text, position + (iconWidth * 2), this.FadeAlphaDuringTransition(color));
        }

        /// <summary>
        /// Draws the 'pending' indication for a given gamer square
        /// </summary>
        /// <param name="playerSeatNumber">The player seat number we are drawing</param>
        private void DrawPendingGamer(byte playerSeatNumber)
        {
            Vector2 position = playerInfoCards[playerSeatNumber];
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            Vector2 iconWidth = new Vector2(34, 0);
            Vector2 iconOffset = new Vector2(0, 12);

            Vector2 iconPosition = position + iconOffset;
            Rectangle backgroundBox = new Rectangle((int)position.X, (int)position.Y, 500, 300);

            spriteBatch.Draw(this.playerBackgroundTexture, backgroundBox, playerColors[playerSeatNumber]);

            // Draw the gamertag, normally in white, but yellow for local players.
            string text = "Press A to join";

            Color color = Color.White;

            spriteBatch.DrawString(font, text, position + (iconWidth * 2), this.FadeAlphaDuringTransition(color));
        }

        /// <summary>
        /// Draws the 'open' indication for a given gamer square
        /// </summary>
        /// <param name="playerSeatNumber">The player seat number we are drawing</param>
        private void DrawOpenGamer(byte playerSeatNumber)
        {
            Vector2 position = playerInfoCards[playerSeatNumber];
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            Vector2 iconWidth = new Vector2(34, 0);
            Vector2 iconOffset = new Vector2(0, 12);

            Vector2 iconPosition = position + iconOffset;
            Rectangle backgroundBox = new Rectangle((int)position.X, (int)position.Y, 500, 300);

            spriteBatch.Draw(this.playerBackgroundTexture, backgroundBox, playerColors[playerSeatNumber]);

            // Draw the gamertag, normally in white, but yellow for local players.
            string text = "Open - Press A to sign in";

            Color color = Color.White;

            spriteBatch.DrawString(font, text, position + (iconWidth * 2), this.FadeAlphaDuringTransition(color));
        }

        /// <summary>
        /// Draws the 'need controller' indication for a given gamer square
        /// </summary>
        /// <param name="playerSeatNumber">The player seat number we are drawing</param>
        private void DrawNeedController(byte playerSeatNumber)
        {
            Vector2 position = playerInfoCards[playerSeatNumber];
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;
            SpriteFont font = ScreenManager.Font;

            Vector2 iconWidth = new Vector2(34, 0);
            Vector2 iconOffset = new Vector2(0, 12);

            Vector2 iconPosition = position + iconOffset;
            Rectangle backgroundBox = new Rectangle((int)position.X, (int)position.Y, 500, 300);

            spriteBatch.Draw(this.playerBackgroundTexture, backgroundBox, playerColors[playerSeatNumber]);

            // Draw the gamertag, normally in white, but yellow for local players.
            string text = "Attach controller to join play";

            Color color = Color.White;

            spriteBatch.DrawString(font, text, position + (iconWidth * 2), this.FadeAlphaDuringTransition(color));
        }

        /// <summary>
        /// Helper modifies a color to fade its alpha value during screen transitions.
        /// </summary>
        /// <param name="color">The color to color this object with during transition</param>
        /// <returns>The color number appropriate for this point of the transition</returns>
        private Color FadeAlphaDuringTransition(Color color)
        {
            return new Color(color.R, color.G, color.B, TransitionAlpha);
        }

        #endregion

        /// <summary>
        /// Handle MenuSelect inputs by marking ourselves as ready.
        /// </summary>
        /// <param name="gamer">The gamer object to mark ready</param>
        private void HandleMenuSelect(LocalNetworkGamer gamer)
        {
            if (!gamer.IsReady)
            {
                if (GameInformation.Instance.AvailableLevels[this.currentLevelNumber].IsLevelUnlocked)
                {
                    gamer.IsReady = true;
                }
            }
            else if (gamer.IsHost)
            {
                // The host has an option to force starting the game, even if not
                // everyone has marked themselves ready. If they press select twice
                // in a row, the first time marks the host ready, then the second
                // time we ask if they want to force start.
                MessageBoxScreen messageBox = new MessageBoxScreen(Resources.ConfirmForceStartGame);

                messageBox.Accepted += this.ConfirmStartGameMessageBoxAccepted;

                ScreenManager.AddScreen(messageBox);
            }
        }

        /// <summary>
        /// Event handler for when the host selects ok on the "are you sure
        /// you want to start even though not everyone is ready" message box.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        private void ConfirmStartGameMessageBoxAccepted(object sender, EventArgs e)
        {
            if (this.networkSession.SessionState == NetworkSessionState.Lobby)
            {
                this.networkSession.StartGame();
            }
        }

        /// <summary>
        /// Handle MenuCancel inputs by clearing our ready status, or if it is
        /// already clear, prompting if the user wants to leave the session.
        /// </summary>
        /// <param name="gamer">The gamer for whom we need to cancel readiness</param>
        private void HandleMenuCancel(LocalNetworkGamer gamer)
        {
            if (gamer.IsReady)
            {
                gamer.IsReady = false;
            }
            else
            {
                NetworkSessionComponent.LeaveSession(ScreenManager, false);
            }
        }

        /// <summary>
        /// Called when a gamer leaves the session
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">The gamer that has just left</param>
        private void NetworkSession_GamerLeft(object sender, GamerLeftEventArgs e)
        {
            if (null != e.Gamer.Tag)
            {
                this.availableSeats[((PlayerInformation)e.Gamer.Tag).SeatNumber] = true;
            }
        }

        /// <summary>
        /// Called when a gamer is added to the session.  This initializes the gamer's selections on the host
        /// and then advises the gamer of the current selections
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Event arguments detailing WHICH gamer has just joined</param>
        private void NetworkSession_GamerJoined(object sender, GamerJoinedEventArgs e)
        {
            // First, grab the new gamer info, and see if it is a local gamer we have been expecting
            LocalNetworkGamer tempGamer = e.Gamer as LocalNetworkGamer;
            if (null != tempGamer)
            {
                this.pendingGamers.Remove(tempGamer.SignedInGamer);
            }

            // If we are the host, we must make a seat assignment.  This is a priority
            if (this.networkSession.IsHost)
            {
                byte playerSeat;

                for (playerSeat = 0; playerSeat < 4; playerSeat++)
                {
                    if (this.availableSeats[playerSeat])
                    {
                        this.availableSeats[playerSeat] = false;
                        break;
                    }
                }

               this.lobbyPacketWriter.Write((byte)0x80);
               this.lobbyPacketWriter.Write(playerSeat);

                // Send the seat number to the current gamer, so that they can send us their requests
                this.networkSession.LocalGamers[0].SendData(this.lobbyPacketWriter, SendDataOptions.ReliableInOrder, e.Gamer);
            }
        }

        /// <summary>
        /// Called by the runtime when a player profile is now available
        /// </summary>
        /// <param name="result">The AsyncResult telling which player just got their profile updated</param>
        private void EndGetProfile(IAsyncResult result)
        {
            try
            {
                NetworkGamer gamer = this.networkSession.FindGamerById((byte)result.AsyncState);
                if (gamer != null)
                {
                    if (gamer.Tag != null)
                    {
                        PlayerInformation playerInfo = (PlayerInformation)gamer.Tag;
                        try
                        {
                            playerInfo.GamerProfile = gamer.EndGetProfile(result);
                        }
                        catch (NetworkException)
                        {
                            // If the gamer is a local gamer on a PC, they don't have a profile
                            // If a gamer doesn't have a profile, you get a Network Exception
                        }
                    }
                }
            }
            catch (GamerPrivilegeException)
            {
            }
        }

        /// <summary>
        /// Fired when a gamer signs in
        /// </summary>
        /// <param name="sender">Sender of the event</param>
        /// <param name="e">Gamer which has signed in</param>
        private void SignedInGamer_SignedIn(object sender, SignedInEventArgs e)
        {
            foreach (LocalNetworkGamer gamer in this.networkSession.LocalGamers)
            {
                if (e.Gamer.Gamertag == gamer.Gamertag)
                {
                    return;
                }
            }

            this.pendingGamers.Add(e.Gamer);
        }

        /// <summary>
        /// Fired when a gamer signs out
        /// </summary>
        /// <param name="sender">Sender of the event</param>
        /// <param name="e">Gamer which has signed out</param>
        private void SignedInGamer_SignedOut(object sender, SignedOutEventArgs e)
        {
            this.pendingGamers.Remove(e.Gamer);
        }

        #region AvatarTintingSelection

        /// <summary>
        /// Given a system color return the CH color number
        /// </summary>
        /// <param name="testingColor">The color to consider</param>
        /// <returns>A CH color index</returns>
        private byte DecodeColor(Color? colorToDecode)
        {
            byte counter = 0;
            foreach (Color? targetColor in characterColors)
            {
                if (colorToDecode == targetColor)
                {
                    return counter;
                }
                counter++;
            }
            return (byte)randomSource.Next(16);
        }

        /// <summary>
        /// Given a player's primary and secondary colors, find a suitable color for them, random if need be
        /// </summary>
        /// <param name="primaryColor">System chosen primary color</param>
        /// <param name="secondaryColor">System chosen secondary color</param>
        /// <param name="seatNumber">The seat number this player is sitting in</param>
        /// <returns>A color index to use when loading the texture sheet</returns>
        private byte SelectPlayerColorIndex(byte primaryColor, byte secondaryColor, int seatNumber)
        {
            byte? colorIndex = null;
            colorIndex = this.CheckColorAgainstPlayers(primaryColor, seatNumber);
            if (colorIndex == null)
            {
                colorIndex = this.CheckColorAgainstPlayers(secondaryColor, seatNumber);
            }

            if (colorIndex == null)
            {
                for (byte counter = 0; counter < 16; counter++)
                {
                    colorIndex = this.CheckColorAgainstPlayers(counter, seatNumber);
                    if (null != colorIndex)
                    {
                        return (byte)colorIndex;
                    }
                }
            }

            if (colorIndex == null)
            {
                return 0;
            }
            else
            {
                return (byte)colorIndex;
            }
        }

        /// <summary>
        /// Given a color index, check the other seats
        /// </summary>
        /// <param name="colorIndex">The index we are testing</param>
        /// <param name="seatNumber">The seat number that would like to use the index</param>
        /// <returns>A valid index, or null</returns>
        private byte? CheckColorAgainstPlayers( byte colorIndex, int seatNumber)
        {
            // We now know what color we would LIKE, but is it AVAILABLE?
            foreach (Gamer testGamer in this.networkSession.AllGamers)
            {
                PlayerInformation playerInfo = ((PlayerInformation)testGamer.Tag);
                if (playerInfo != null)
                {
                    if (playerInfo.SeatNumber != seatNumber)
                    {
                        if (playerInfo.PlayerColorIndex == colorIndex)
                        {
                            // Crap, we found another player who is already using our color
                            return null;
                        }
                    }
                }
            }

            return colorIndex;
        }

        #endregion

    }
}
