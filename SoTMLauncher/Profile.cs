using System;
using System.IO;
using Semver;

namespace SotMLauncher
{
    public class Profile
    {
        public string GameDirPath = "";
        public SemVersion LocalTaggedSemVer = "0.0.0";
        public bool UseDebugBuild = false;

        public Profile() { }

        public string GetFilePath()
        {
            string fileName = Main.MOD_RELATIVE_PATH + "config" + Main.PROFILE_FILE_EXT;
            string filePath;
            try
            {
                filePath = Path.GetFullPath(fileName);
            }
            catch (PathTooLongException)
            {
                filePath = Path.GetFullPath(Main.MOD_RELATIVE_PATH + (Path.GetRandomFileName().Split('.')[0]) + Main.PROFILE_FILE_EXT);
            }
            return filePath;
        }

        public bool Serialize(BinaryWriter writer)
        {
            bool modsSuccessfullySerialized = true;
            try
            {
                writer.Write(GameDirPath);
                writer.Write(LocalTaggedSemVer.ToString());
                writer.Write(UseDebugBuild);
            }
            catch (Exception ex)
            {
                System.Windows.Forms.MessageBox.Show("Error: could not serialize Profile\n" + ex.Message + "\n\n" + ex.StackTrace, Main.MESSAGEBOX_CAPTION, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Error);
                return false;
            }
            return modsSuccessfullySerialized;
        }

        public bool Deserialize(BinaryReader reader)
        {
            try
            {
                GameDirPath = reader.ReadString();
                string parsedSemver = reader.ReadString();
                UseDebugBuild = reader.ReadBoolean();

                bool isValidSemver = SemVersion.TryParse(parsedSemver, out LocalTaggedSemVer);
                if (!isValidSemver)
                {
                    Logger.Log.WriteError("Could not parse semver from profile");
                    return false;
                }
            }
            catch (Exception ex)
            {
                System.Windows.Forms.MessageBox.Show("Error: could not deserialize Profile\n" + ex.Message + "\n\n" + ex.StackTrace, Main.MESSAGEBOX_CAPTION, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Error);
                return false;
            }
            return true;
        }
    }
}
