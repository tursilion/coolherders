//-----------------------------------------------------------------------------
// <copyright file="DiscoLevel.cs" company="HarmlessLion">
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
    /// This is a compiled in level for the Disco
    /// </summary>
    public class DiscoLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the DiscoLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public DiscoLevel()
        {
            this.theLevelName = "disco";
            this.thePrintableName = "Disco Hall";
            this.theLevelNumber = 4;
            this.theBackgroundSongName = "DiscoBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = true;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Backup";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 4;
            this.SinglePlayerEnemies.Add(enemyPlayer);
            enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Backup";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 2;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 5;
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
