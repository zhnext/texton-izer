import os
import string

def main():
    t = Tester("tester.conf")
    t.run_tests();
    

class Tester():
    """A testing class
    
    There are two important configuration files:
    1. Configures the binary, output dir, and test configuration file(s)
    2. Configures a test run parameters for the binary
    """
    def __init__(self, tester_conf_path):
        [self.binary_path, self.out_path, self.conf_file] = self.parse_conf(tester_conf_path)
        self.test_params = []

    def get_test_images(self):
        """Read conf_file and parse it to get image names and params
        
        Each line of the config file should look look this:
        img_file_path num_of_clusters
        """
        self.test_params = []
        fp = open(self.conf_file)
        for line in fp.readlines():
            tmp = string.strip(line).split("|")
            print tmp
            self.test_params.append(tmp)
        fp.close()
            
    def parse_conf(self, tester_conf_path):
        paths = []
        fp = open(tester_conf_path)
        for line in fp.readlines():
            paths.append(string.strip(line))
        fp.close();
        return paths
    
    def run_tests(self):
        """Run tests
        """
        self.get_test_images()
        for elem in self.test_params:            
            basename = os.path.basename(elem[0])
            filename = os.path.splitext(basename)
            img_name = filename[0]
            out_dir = self.out_path+img_name+"\\";
            try:
                os.mkdir(out_dir)
                exec_str = "%s %s %s %s" % (self.binary_path, elem[0], out_dir, elem[1])
                print "Executing: " + exec_str
                os.system(exec_str)
            except OSError:
                raise
    
if __name__ == '__main__':   
     main()