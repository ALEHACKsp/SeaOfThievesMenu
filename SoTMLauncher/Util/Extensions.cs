using Semver;
using System.IO;

namespace SotMLauncher.Util
{
    public static class ExtensionMethods
    {
        public static string ToSafeFileName(this SemVersion semver)
        {
            string semVerString = semver.ToString();
            semVerString = semVerString.Replace('.', '_');

            foreach (char invalidChar in Path.GetInvalidFileNameChars())
            {
                semVerString.Replace(invalidChar, '_');
            }

            return semVerString;
        }
    }
}
