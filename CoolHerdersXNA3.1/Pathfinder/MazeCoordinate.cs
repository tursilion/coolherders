//-----------------------------------------------------------------------------
// <copyright file="MazeCoordinate.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Pathfinder
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// A coordinate within the maze, with the additional score information for use by the A* pathfinder
    /// </summary>
    internal class MazeCoordinate
    {
        /// <summary>
        /// The cell column of this routing tile
        /// </summary>
        public int CellColumn = 0;

        /// <summary>
        /// The cell row of this routing tile
        /// </summary>
        public int CellRow = 0;

        /// <summary>
        /// The F score for A* routing
        /// </summary>
        public int ScoreF = 0;

        /// <summary>
        /// The G score for A* routing
        /// </summary>
        public int ScoreG = 0;

        /// <summary>
        /// The H score for A* routing
        /// </summary>
        public int ScoreH = 0;

        /// <summary>
        /// The parent cell for A* routing
        /// </summary>
        public MazeCoordinate ParentCell = null;

        /// <summary>
        /// Initializes a new instance of the MazeCoordinate class
        /// </summary>
        public MazeCoordinate()
        {
        }

        /// <summary>
        /// Resets a maze coordinate, as it may have leftover junk from the pool
        /// </summary>
        /// <param name="column">The column to specify</param>
        /// <param name="row">The row to specify</param>
        public void ResetMazeCoordinate(int column, int row)
        {
            this.CellColumn = column;
            this.CellRow = row;
            this.ScoreF = 0;
            this.ScoreG = 0;
            this.ScoreH = 0;
            this.ParentCell = null;
        }
    }
}
