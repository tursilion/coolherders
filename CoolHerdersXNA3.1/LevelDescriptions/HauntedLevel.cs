//-----------------------------------------------------------------------------
// <copyright file="HauntedLevel.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// This is a compiled in level for the haunted house
    /// </summary>
    public class HauntedLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the HauntedLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public HauntedLevel()
        {
            this.theLevelName = "haunted";
            this.thePrintableName = "Haunted House";
            this.theLevelNumber = 2;
            this.theBackgroundSongName = "HauntedHouseBackground";
            this.sheepAreGhosts = true;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Hades";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 2;
            this.SinglePlayerEnemies.Add(enemyPlayer);
        }

        public override bool IsLevelUnlocked
        {
            get
            {
                return true;
            }
        }
    }
}
