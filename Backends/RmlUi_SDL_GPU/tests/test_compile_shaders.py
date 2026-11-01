"""Standalone regression for atomic shader-header publication."""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest


class ShaderPublicationTest(unittest.TestCase):
    def test_failure_preserves_header_and_success_replaces_it(self):
        script = pathlib.Path(__file__).resolve().parents[1] / 'compile_shaders.py'
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            shutil.copy(script, root / script.name)
            (root / 'example.vert').write_text('shader')
            header = root / 'ShadersCompiledSPV.h'
            header.write_text('previous header')
            compiler = root / 'shadercross'
            compiler.write_text('#!' + sys.executable + '\n'
                'import pathlib, sys\n'
                'target = sys.argv[sys.argv.index("-d") + 1]\n'
                'if target == "msl" and (pathlib.Path(__file__).parent / "fail").exists(): sys.exit(1)\n'
                'pathlib.Path(sys.argv[sys.argv.index("-o") + 1]).write_bytes(b"\\x01\\x02")\n')
            compiler.chmod(0o755)
            (root / 'fail').touch()
            command = [sys.executable, str(root / script.name), str(compiler)]
            failed = subprocess.run(command, capture_output=True)
            self.assertNotEqual(failed.returncode, 0)
            self.assertEqual(header.read_text(), 'previous header')
            (root / 'fail').unlink()
            success = subprocess.run(command, capture_output=True)
            self.assertEqual(success.returncode, 0, success.stderr.decode())
            result = header.read_text()
            for target in ('spirv', 'msl', 'dxil'):
                self.assertIn('example_' + target, result)
            self.assertIn('0x01,0x02,', result)


if __name__ == '__main__':
    unittest.main()
