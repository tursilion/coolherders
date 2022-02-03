//-----------------------------------------------------------------------------
// <copyright file="GameInformation.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Housekeeping
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using CoolHerders.StoryMode;
    using CoolHerders.LevelDescriptions;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    using Microsoft.Xna.Framework.Storage;

    /// <summary>
    /// A repository of information about the game which is currently being played
    /// </summary>
    internal sealed class GameInformation
    {
        /// <summary>
        /// Initializes the singleton instance when needed
        /// </summary>
        private static readonly GameInformation instance = new GameInformation();

        /// <summary>
        /// The master player index, for things like Single Player mode.
        /// This is determined when you press 'start' at the title screen
        /// </summary>
        private PlayerIndex masterPlayerIndex;

        /// <summary>
        /// The description of the world screen that is currently running
        /// </summary>
        private LevelDescription currentWorldScreen;

        /// <summary>
        /// The stage within the world that we are using
        /// </summary>
        private int currentStageNumber;

        public int CurrentStageNumber
        {
            get { return currentStageNumber; }
            set { currentStageNumber = value; }
        }

        private PacketWriter packetWriter;

        public PacketWriter GamePacketWriter
        {
            get { return packetWriter; }
            set { packetWriter = value; }
        }

        private PacketReader packetReader;

        public PacketReader GamePacketReader
        {
            get { return packetReader; }
            set { packetReader = value; }
        }

        /// <summary>
        /// The 'script processor' that handles the story mode screens
        /// </summary>
        private StoryModeSequencer storyModeSequencer;

        /// <summary>
        /// Any enemy players that may be in the level with us
        /// </summary>
        private List<PlayerInformation> enemyPlayers;

        private List<LevelDescription> availableLevels;

        public List<LevelDescription> AvailableLevels
        {
            get { return availableLevels; }
            set { availableLevels = value; }
        }

        /// <summary>
        /// Prevents a default instance of the GameInformation class from being created
        /// </summary>
        private GameInformation()
        {
            this.currentWorldScreen = null;
            this.availableLevels = LevelDescription.LoadAllLevelsFromCode();
            SignedInGamer.SignedIn += new EventHandler<SignedInEventArgs>(SignedInGamer_SignedIn);
        }

        void SignedInGamer_SignedIn(object sender, SignedInEventArgs e)
        {
            e.Gamer.Tag = new PlayerSaveManager(e.Gamer.PlayerIndex);
        }

        /// <summary>
        /// Gets the gane information singleton instance
        /// </summary>
        public static GameInformation Instance
        {
            get
            {
                return instance;
            }
        }

        public LevelDescription WorldScreen
        {
            get
            {
                return this.currentWorldScreen;
            }

            set
            {
                this.currentWorldScreen = value;
            }
        }

        /// <summary>
        /// Gets and sets the master player index, which shows which controller pressed 'start' at the title screen
        /// </summary>
        public PlayerIndex MasterPlayerIndex
        {
            get { return masterPlayerIndex; }
            set { masterPlayerIndex = value; }
        }

        /// <summary>
        /// Gets or sets the list of enemy players that are being faught this game
        /// </summary>
        internal List<PlayerInformation> EnemyPlayers
        {
            get { return this.enemyPlayers; }
            set { this.enemyPlayers = value; }
        }

        internal StoryModeSequencer StoryModeSequencer
        {
            get { return storyModeSequencer; }
            set { storyModeSequencer = value; }
        }
    }
}
