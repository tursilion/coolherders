using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.GamerServices;
using Microsoft.Xna.Framework.Storage;

namespace CoolHerders.Housekeeping
{
    internal class PlayerSaveManager
    {
        private StorageDevice currentDevice;

        private PlayerSavedInformation savedInfo;

        public PlayerSavedInformation SavedInfo
        {
            get { return savedInfo; }
            set { savedInfo = value; }
        }

        private PlayerIndex playerIndex;

        private bool loadRequested;

        private bool saveRequested;

        public bool SaveRequested
        {
            get { return saveRequested; }
        }

        public bool LoadRequested
        {
            get { return loadRequested; }
        }

        public PlayerSaveManager (PlayerIndex playerIndex)
        {
            this.playerIndex = playerIndex;
        }

        public void LoadSavedGame ()
        {
            if ((this.currentDevice == null) || (!this.currentDevice.IsConnected))
            {
                // If we do not have a valid storage device
                if (!Guide.IsVisible)
                {
                    Debug.WriteLine("Displaying the device selector for a load.");
                    this.loadRequested = true;
                    // XNA4.0 - replaced by StorageDevice methods
                    //Guide.BeginShowStorageDeviceSelector(this.playerIndex, DeviceReady, null);
                    StorageDevice.BeginShowSelector(this.playerIndex, DeviceReady, null);
                }
            }
            else
            {
                Debug.WriteLine("Assuming the device selector for a load.");
                this.loadRequested = true;
                DoLoadGame();
            }
        }

        public void SaveGame()
        {
            if ((this.currentDevice == null) || (!this.currentDevice.IsConnected))
            {
                // If we do not have a valid storage device
                if (!Guide.IsVisible)
                {
                    Debug.WriteLine("Displaying the device selector for a save.");
                    this.saveRequested = true;
                    // XNA4.0 - replaced with StorageDevice methods
                    //Guide.BeginShowStorageDeviceSelector(this.playerIndex, DeviceReady, null);
                    StorageDevice.BeginShowSelector(this.playerIndex, DeviceReady, null);
                }
            }
            else
            {
                Debug.WriteLine("Assuming the device selector for a save.");
                this.saveRequested = true;
                DoSaveGame();
            }
        }

        void DeviceReady(IAsyncResult result)
        {
            Debug.WriteLine("The device is ready.");
            // XNA4.0 replaced with StorageDevice methods
            //this.currentDevice = Guide.EndShowStorageDeviceSelector(result);
            this.currentDevice = StorageDevice.EndShowSelector(result);
            if ((this.currentDevice != null) && (this.currentDevice.IsConnected))
            {
                Debug.WriteLine("The device is valid.");
                if (this.LoadRequested)
                {
                    DoLoadGame();
                }
                if (this.saveRequested)
                {
                    DoSaveGame();
                }
            }
        }

        private const string SaveFileName = "CHdolphinvanilla.sav";

        protected void LoadStorageCallback(IAsyncResult r)
        {
            StorageContainer loadContainer;

            loadContainer = this.currentDevice.EndOpenContainer(r);

            Stream savegameStream = null;
            try
            {
                Debug.WriteLine("Attempting to open file.");
                savegameStream = loadContainer.OpenFile(SaveFileName, FileMode.Open);
            }
            catch (FileNotFoundException)
            {
                Debug.WriteLine("File does not exist.");
                this.loadRequested = false;
                loadContainer.Dispose();
                return;
            }

            XmlSerializer savegameDeserializer = new XmlSerializer(typeof(PlayerSavedInformation));
            try
            {
                Debug.WriteLine("Attempting to read file.");
                SavedInfo = (PlayerSavedInformation)savegameDeserializer.Deserialize(savegameStream);
            }
            catch (InvalidOperationException)
            {
                Debug.WriteLine("File contains corrupted data.");
            }

            savegameStream.Close();
            this.loadRequested = false;
            loadContainer.Dispose();
        }

        void DoLoadGame()
        {
            try
            {
                Debug.WriteLine("Attempting to open container.");
                // XNA4.0 this whole thing is asynchronous now. whee.
                //loadContainer = this.currentDevice.OpenContainer("CoolHerders");
                this.currentDevice.BeginOpenContainer("CoolHerders", LoadStorageCallback, null);
            }
            catch (InvalidOperationException)
            {
                Debug.WriteLine("Container does not exist.");
                return;
            }
        }

        protected void SaveStorageCallback(IAsyncResult r)
        {
            StorageContainer saveContainer;
            saveContainer = this.currentDevice.EndOpenContainer(r);

            Stream savegameStream = saveContainer.CreateFile(SaveFileName);
            XmlSerializer savegameSerializer = new XmlSerializer(typeof(PlayerSavedInformation));
            savegameSerializer.Serialize(savegameStream, SavedInfo);
            savegameStream.Close();

            Debug.WriteLine(string.Format("File has been saved {0} - {1}", this.savedInfo.lastCompletedLevel, this.savedInfo.highestCompletedLevel));
            this.saveRequested = false;
            saveContainer.Dispose();
        }

        void DoSaveGame()
        {
            try
            {
                Debug.WriteLine("Attempting to open container.");
                // XNA4.0 - this is all changed to async
//                saveContainer = this.currentDevice.OpenContainer("CoolHerders");
                this.currentDevice.BeginOpenContainer("CoolHerders", SaveStorageCallback, null);
            }
            catch (InvalidOperationException)
            {
                Debug.WriteLine("Cannot create a save container.");
                return;
            }
        }
    }
}