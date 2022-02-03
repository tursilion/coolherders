//-----------------------------------------------------------------------------
// <copyright file="HeavenLevel.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// This is a compiled in level for Heaven
    /// </summary>
    public class HeavenLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the HeavenLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public HeavenLevel()
        {
            this.theLevelName = "heaven";
            this.thePrintableName = "Heaven";
            this.theLevelNumber = 6;
            this.theBackgroundSongName = "HeavenFinalBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Angel";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 9;
            this.SinglePlayerEnemies.Add(enemyPlayer);
            enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Angel";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 2;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 10;
            this.SinglePlayerEnemies.Add(enemyPlayer);
            enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Angel";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 3;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 10;
            this.SinglePlayerEnemies.Add(enemyPlayer);
        }

        public override bool IsLevelUnlocked
        {
            get
            {
                return LockByHighestLevel();
            }
        }
    }
}
