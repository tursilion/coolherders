//-----------------------------------------------------------------------------
// <copyright file="LevelDescription.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using CoolHerders.Housekeeping; 
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// A list of levels that are compiled in to the main block of code
    /// </summary>
    public enum BuiltInLevel
    {
        /// <summary>
        /// The New Zealand level
        /// </summary>
        LevelNewZealand,

        /// <summary>
        /// The Candyland level
        /// </summary>
        LevelCandy,

        /// <summary>
        /// The Haunted House level
        /// </summary>
        LevelHauntedHouse,

        /// <summary>
        /// The toy shop level
        /// </summary>
        LevelToyshop,

        /// <summary>
        /// The Disco level
        /// </summary>
        LevelDisco,

        /// <summary>
        /// The WaterWorks level
        /// </summary>
        LevelWaterworks,

        /// <summary>
        /// The Hell level
        /// </summary>
        LevelHell,

            /// <summary>
        /// The Heaven level
        /// </summary>
        LevelHeaven,
    }

    /// <summary>
    /// This class tracks all the levels compiled in and loaded from an expansion pack (some day?!)
    /// </summary>
    public class LevelDescription
    {
        const string levelLetters = "abc";

        /// <summary>
        /// The name of the level for content manager needs
        /// </summary>
        protected string theLevelName;

        protected string thePrintableName;

        public string DisplayName
        {
            get { return thePrintableName; }
        }

        /// <summary>
        /// The level number
        /// </summary>
        protected int theLevelNumber;

        /// <summary>
        /// The name of the song to play in the background
        /// </summary>
        protected string theBackgroundSongName;

        /// <summary>
        /// The next map to run in Multiplayer mode
        /// </summary>
        protected LevelDescription nextMultiplayerLevel;

        /// <summary>
        /// Sheep will appear ghosted when this level is played
        /// </summary>
        protected bool sheepAreGhosts;

        /// <summary>
        /// Zeus will be wearing his 'fro when this level is played
        /// </summary>
        protected bool zeusWearsAFro;

        /// <summary>
        /// AI enemy types to put in for single player mode
        /// </summary>
        protected List<PlayerInformation> singlePlayerEnemies;

        /// <summary>
        /// Initializes a new instance of the LevelDescription class
        /// </summary>
        public LevelDescription()
        {
        }

        /// <summary>
        /// Gets the name of the level for content manager needs
        /// </summary>
        public string LevelName
        {
            get
            {
                return this.theLevelName;
            }
        }

        /// <summary>
        /// Gets the name of the map for content manager needs
        /// </summary>
        public string GetLevelMapName (int mapNumber)
        {
            return string.Format(CultureInfo.InvariantCulture, "Stages\\{0}\\level{1}{2}", this.theLevelName, this.theLevelNumber, levelLetters[mapNumber]);
        }

        public virtual bool IsLevelUnlocked
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating the level number
        /// </summary>
        public int LevelNumber
        {
            get
            {
                return this.theLevelNumber;
            }
        }

        /// <summary>
        /// Gets a value indicating the background song name
        /// </summary>
        public string BackgroundSongName
        {
            get
            {
                return this.theBackgroundSongName;
            }
        }

        /// <summary>
        /// Gets a value indicating if the sheep are to appear ghosty
        /// </summary>
        public bool SheepAreGhosted
        {
            get
            {
                return this.sheepAreGhosts;
            }
        }

        /// <summary>
        /// Gets a value indicating if zeus will be wearing his fro
        /// </summary>
        public bool ZeusWearsAFro
        {
            get
            {
                return this.zeusWearsAFro;
            }
        }

        internal List<PlayerInformation> SinglePlayerEnemies
        {
            get
            {
                return singlePlayerEnemies;
            }
        }

        /// <summary>
        /// Loads a level that was compiled into the code and main content manager
        /// </summary>
        /// <param name="level">The level that we are trying to load</param>
        /// <param name="mapNumber">The map number within that level we wish to display</param>
        /// <returns>The level description for the requested level</returns>
        public static LevelDescription LoadLevelFromCode(BuiltInLevel level)
        {
            switch (level)
            {
                case BuiltInLevel.LevelCandy:
                    return new CandyLevel();
                case BuiltInLevel.LevelDisco:
                    return new DiscoLevel();
                case BuiltInLevel.LevelHauntedHouse:
                    return new HauntedLevel();
                case BuiltInLevel.LevelHeaven:
                    return new HeavenLevel();
                case BuiltInLevel.LevelHell:
                    return new HellLevel();
                case BuiltInLevel.LevelNewZealand:
                    return new NewZealandLevel();
                case BuiltInLevel.LevelToyshop:
                    return new ToyLevel();
                case BuiltInLevel.LevelWaterworks:
                    return new WaterLevel();
            }

            throw new ArgumentOutOfRangeException("level", "Level number must be in the range 0-7");
        }

        /// <summary>
        /// Creates a list of all levels which have been compiled into the code
        /// </summary>
        /// <returns>A List<> of level descriptions</returns>
        public static List<LevelDescription> LoadAllLevelsFromCode() {
            List<LevelDescription> levelList = new List<LevelDescription>();
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelNewZealand));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelCandy));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelHauntedHouse));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelToyshop));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelDisco));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelWaterworks));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelHell));
            levelList.Add(LoadLevelFromCode(BuiltInLevel.LevelHeaven));
            return levelList;
        }

        protected bool LockByHighestLevel()
        {
            if (Guide.IsTrialMode)
            {
                return false;
            }
            else
            {
                foreach (SignedInGamer gamer in SignedInGamer.SignedInGamers)
                {
                    PlayerSavedInformation saveInformation = ((PlayerSaveManager)gamer.Tag).SavedInfo;
                    if (saveInformation.highestCompletedLevel >= this.theLevelNumber)
                    {
                        return true;
                    }
                }
                return false;
            }
        }
    }
}
