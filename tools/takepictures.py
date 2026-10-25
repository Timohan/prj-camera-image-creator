#!/usr/bin/env python3
"""Takes the pictures of every objects/*.dat file into outputDirectory of configs/general.ini:
usage takepictures.py [--t<number of threads>], for example --t10 runs 10 programs at the same time, default is 1.
Previous files of outputDirectory are moved to /tmp/image-creator-<timestamp> first. If all pictures are created,
the previous files are removed, if any objects file fails, the new files are removed and the previous files are
moved back to outputDirectory."""
import os, re, shutil, subprocess, sys, threading, time
from concurrent.futures import ThreadPoolExecutor

# repository root, this script is in the tools folder
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROGRAM = os.path.join(REPO, 'prj-camera-image-creator')
GENERAL_INI = os.path.join(REPO, 'configs', 'general.ini')
OBJECTS_DIRECTORY = os.path.join(REPO, 'objects')
BACKUP_PREFIX = '/tmp/image-creator-'


def read_output_directory():
    """outputDirectory of general.ini, relative path is relative to the repository like for the program"""
    for line in open(GENERAL_INI):
        line = line.strip()
        if line.startswith('outputDirectory='):
            directory = line[len('outputDirectory='):].strip()
            if directory:
                return os.path.abspath(os.path.join(REPO, os.path.expanduser(directory)))
    return None


def check_output_directory(directory):
    """all files of the directory are moved away, so it must not contain the repository or the home directory"""
    for important in ('/', os.path.expanduser('~'), REPO):
        important = os.path.realpath(important)
        real = os.path.realpath(directory)
        if important == real or important.startswith(real.rstrip('/') + '/'):
            return False
    return True


def find_objects_files():
    """objects files sorted by the number, objects2.dat before objects10.dat"""
    names = [name for name in os.listdir(OBJECTS_DIRECTORY)
             if name.endswith('.dat') and os.path.isfile(os.path.join(OBJECTS_DIRECTORY, name))]
    return sorted(names, key=lambda name: [int(part) if part.isdigit() else part for part in re.split(r'(\d+)', name)])


def move_all(source, destination):
    for name in os.listdir(source):
        shutil.move(os.path.join(source, name), os.path.join(destination, name))


def remove_all(directory):
    for name in os.listdir(directory):
        path = os.path.join(directory, name)
        if os.path.isdir(path) and not os.path.islink(path):
            shutil.rmtree(path)
        else:
            os.remove(path)


def read_thread_count(arguments):
    """--t<number> from the arguments, 1 if it is missing, None if the arguments are invalid"""
    threads = 1
    for argument in arguments:
        match = re.fullmatch(r'--t(\d+)', argument)
        if not match or int(match.group(1)) < 1:
            return None
        threads = int(match.group(1))
    return threads


class PictureTaker:
    """runs the program for every objects file, at most thread count programs at the same time,
    the first failure stops all programs"""
    def __init__(self, objects_files, output_directory):
        self.objects_files = objects_files
        self.output_directory = output_directory
        self.lock = threading.Lock()
        self.processes = set()
        self.stopped = False
        self.failed = None
        self.done = 0

    def stop(self, failed):
        with self.lock:
            if self.stopped:
                return
            self.stopped = True
            self.failed = failed
            for process in self.processes:
                process.kill()

    def take_pictures(self, objects_file):
        """runs the program for one objects file, any error stops all programs at once"""
        try:
            self.run_program(objects_file)
        except Exception as error:
            self.stop('%s failed: %s' % (objects_file, error))

    def run_program(self, objects_file):
        command = [PROGRAM, '--takePictures', '--camerasVisible=0', '--objectsFile=' + objects_file]
        with self.lock:
            if self.stopped:
                return
            process = subprocess.Popen(command, cwd=REPO, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            self.processes.add(process)
        output = process.communicate()[0]
        with self.lock:
            self.processes.discard(process)
            if self.stopped:
                return
        error = None
        if process.returncode != 0:
            error = 'exit code %d' % process.returncode
        else:
            # program exits without error also when the pictures cannot be saved, so check that they exist
            stem = os.path.splitext(objects_file)[0] + '_'
            saved = [name for name in os.listdir(self.output_directory) if name.startswith(stem)]
            if not any(name.endswith('.png') for name in saved) or not any(name.endswith('.dat') for name in saved):
                error = 'no pictures were saved'
        if error:
            print(output, end='', flush=True)
            self.stop('%s failed: %s' % (objects_file, error))
            return
        with self.lock:
            self.done += 1
            print('[%d/%d] %s is done' % (self.done, len(self.objects_files), objects_file), flush=True)

    def run(self, threads):
        """returns error text or None on success"""
        executor = ThreadPoolExecutor(max_workers=threads)
        futures = [executor.submit(self.take_pictures, name) for name in self.objects_files]
        try:
            for future in futures:
                future.result()
        except KeyboardInterrupt:
            self.stop('interrupted')
        executor.shutdown(wait=True)
        return self.failed


def main():
    threads = read_thread_count(sys.argv[1:])
    if threads is None:
        print('usage: takepictures.py [--t<number of threads>], for example --t10')
        return 1
    output_directory = read_output_directory()
    if not output_directory:
        print('Error: outputDirectory is not set in %s' % GENERAL_INI)
        return 1
    if not check_output_directory(output_directory):
        print('Error: outputDirectory %s contains the home directory or the repository, nothing is done' % output_directory)
        return 1
    if not os.access(PROGRAM, os.X_OK):
        print('Error: %s is missing, build it with make' % PROGRAM)
        return 1
    objects_files = find_objects_files()
    if not objects_files:
        print('Error: no .dat files in %s' % OBJECTS_DIRECTORY)
        return 1

    os.makedirs(output_directory, exist_ok=True)
    backup_directory = BACKUP_PREFIX + time.strftime('%Y%m%d-%H%M%S')
    os.makedirs(backup_directory)
    move_all(output_directory, backup_directory)
    print('Previous files of %s are moved to %s' % (output_directory, backup_directory))

    print('Taking pictures of %d objects files with %d threads' % (len(objects_files), threads), flush=True)
    failed = PictureTaker(objects_files, output_directory).run(threads)

    if failed:
        remove_all(output_directory)
        move_all(backup_directory, output_directory)
        os.rmdir(backup_directory)
        print('Error: %s' % failed)
        print('no previous images are removed')
        return 1

    shutil.rmtree(backup_directory)
    print('All files are created into %s, all previous images are removed' % output_directory)
    return 0


if __name__ == '__main__':
    sys.exit(main())
