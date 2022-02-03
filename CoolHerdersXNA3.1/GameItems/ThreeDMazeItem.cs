//-----------------------------------------------------------------------------
// <copyright file="ThreeDMazeItem.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// This tracks a maze item that needs to be positioned in the pseudo-3D space
    /// </summary>
    public class ThreeDMazeItem : MazeItem
    {
        /// <summary>
        /// Initializes a new instance of the ThreeDMazeItem class
        /// </summary>
        /// <param name="parentComponent">The parent component that will draw this maze item</param>
        /// <param name="resourceDirectory">The directory in which the tiles for this item can be found</param>
        /// <param name="levelNumber">The level number that we will be drawn</param>
        /// <param name="mazeTile">The maze tile object with information about this item</param>
        /// <param name="rowCounter">The row counter this item is placed at</param>
        /// <param name="columnCounter">The column counter this item is placed at</param>
        internal ThreeDMazeItem(MazeGameComponent parentComponent, string resourceDirectory, int levelNumber, MazeMap.MazeTile mazeTile, int rowCounter, int columnCounter) : base(parentComponent, resourceDirectory, levelNumber, mazeTile, rowCounter, columnCounter)
        {
            this.itemDepth = (rowCounter * 0.05f) + (columnCounter * 0.002f);
        }
    }
}
