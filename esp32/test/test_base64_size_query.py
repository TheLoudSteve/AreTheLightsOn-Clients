"""Regression guard for the ESP32 provisioning Base64 size query."""

from pathlib import Path
import unittest


SOURCE = Path(__file__).parents[1] / "src" / "main.cpp"


class Base64SizeQueryTest(unittest.TestCase):
    def test_accepts_mbedtls_size_query_result(self):
        source = SOURCE.read_text()
        self.assertIn(
            "sizeResult != 0 && sizeResult != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL",
            source,
        )
        self.assertIn("if (outputLength == 0) return \"\";", source)


if __name__ == "__main__":
    unittest.main()
