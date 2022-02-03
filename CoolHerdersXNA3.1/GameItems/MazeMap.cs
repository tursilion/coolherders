//-----------------------------------------------------------------------------
// <copyright file="MazeMap.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// Holds the in memory representation of a single maze map
    /// </summary>
    public class MazeMap
    {
        /// <summary>
        /// The number of tiles wide the maze is
        /// </summary>
        private int mazeXDimension;

        /// <summary>
        /// The number of tiles tall the maze is
        /// </summary>
        private int mazeYDimension;

        /// <summary>
        /// The list of all tiles in the maze
        /// </summary>
        private List<MazeTile> mazeTileList;

        /// <summary>
        /// Initializes a new instance of the MazeMap class.
        /// </summary>
        /// <param name="mazeXDimension">The X dimension of the maze in columns</param>
        /// <param name="mazeYDimension">The Y dimension of the maze in rows</param>
        public MazeMap(int mazeXDimension, int mazeYDimension)
        {
            this.mazeXDimension = mazeXDimension;
            this.mazeYDimension = mazeYDimension;
            this.mazeTileList = new List<MazeTile>(mazeXDimension * mazeYDimension);
        }

        /// <summary>
        /// Gets the number of columns in the maze
        /// </summary>
        public int MazeXDimension
        {
            get
            {
                return this.mazeXDimension;
            }
        }

        /// <summary>
        /// Gets the number of rows in the maze
        /// </summary>
        public int MazeYDimension
        {
            get
            {
                return this.mazeYDimension;
            }
        }

        /// <summary>
        /// Gets the list of tiles in the maze
        /// </summary>
        public List<MazeTile> MazeTiles
        {
            get
            {
                return this.mazeTileList;
            }
        }

        /// <summary>
        /// Adds a tile to the maze at the next available position
        /// </summary>
        /// <param name="nextTile">The next tile in the maze, going first by column, then by row</param>
        public void AddTile(MazeTile nextTile)
        {
            this.mazeTileList.Add(nextTile);
        }

        /// <summary>
        /// Holds the in memory representation of the specification for a given tile
        /// </summary>
        public class MazeTile
        {
            /// <summary>
            /// Is this tile destructable?
            /// </summary>
            private bool isDestructable;

            /// <summary>
            /// Is this tile passable?
            /// </summary>
            private bool isPassable;

            /// <summary>
            /// Is this tile 3D
            /// </summary>
            private bool is3D;

            /// <summary>
            /// What image number to draw for this tile?
            /// </summary>
            private int imageNumber;

            /// <summary>
            /// What tileset to draw the image from?
            /// </summary>
            private int imageTileSet;

            /// <summary>
            /// Some tiles will have another tile displayed underneath.  This is the image number to display beneath.
            /// </summary>
            private int underneathImageNumber;

            /// <summary>
            /// Some tiles will have another tile displayed underneath.  This is the tileset to draw that from.
            /// </summary>
            private int underneathTileSet;

            /// <summary>
            /// Initializes a new instance of the MazeTile class.
            /// </summary>
            /// <param name="imageTileSet">Tile set number</param>
            /// <param name="imageNumber">Image number within the tileset</param>
            /// <param name="isPassable">Is this tile passable</param>
            /// <param name="isDestructable">Is this tile destructable</param>
            /// <param name="is3D">Is this tile to be drawn in pseudo 3D</param>
            /// <param name="underneathImageNumber">The tile number to draw beneath the main tile</param>
            /// <param name="underneathTileSet">The tileset number to draw underneath the main tile</param>
            public MazeTile(int imageTileSet, int imageNumber, bool isPassable, bool isDestructable, bool is3D, int underneathImageNumber, int underneathTileSet)
            {
                this.isDestructable = isDestructable;
                this.isPassable = isPassable;
                this.is3D = is3D;
                this.imageNumber = imageNumber;
                this.imageTileSet = imageTileSet;
                this.underneathImageNumber = underneathImageNumber;
                this.underneathTileSet = underneathTileSet;
            }

            /// <summary>
            /// Gets the tile set to draw this image from
            /// </summary>
            public int ImageTileSet
            {
                get
                {
                    return this.imageTileSet;
                }
            }

            /// <summary>
            /// Gets the image number to draw from the given tile set
            /// </summary>
            public int ImageNumber
            {
                get
                {
                    return this.imageNumber;
                }
            }

            /// <summary>
            /// Gets a value indicating whether this tile is passable or not
            /// </summary>
            public bool IsPassable
            {
                get
                {
                    return this.isPassable;
                }
            }

            /// <summary>
            /// Gets a value indicating whether this tile is destructable or not
            /// </summary>
            public bool IsDestructable
            {
                get
                {
                    return this.isDestructable;
                }
            }

            /// <summary>
            /// Gets a value indicating whether this tile is 3D or not
            /// </summary>
            public bool Is3D
            {
                get
                {
                    return this.is3D;
                }
            }

            /// <summary>
            /// Gets the image number to draw underneath this tile
            /// </summary>
            public int UnderneathImageNumber
            {
                get
                {
                    return this.underneathImageNumber;
                }
            }

            /// <summary>
            /// Gets the tileset to use to draw underneath this tile
            /// </summary>
            public int UnderneathTileSet
            {
                get
                {
                    return this.underneathTileSet;
                }
            }
        }
    }
}
