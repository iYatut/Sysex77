import { execSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const input = path.join(root, 'fixtures', 'paramMeta.sample.json');
const output = path.join(root, 'src', 'types', 'paramMeta.generated.ts');

execSync(
  `npx quicktype "${input}" -o "${output}" --src-lang json --lang typescript --top-level ParamMeta --just-types`,
  { cwd: root, stdio: 'inherit' },
);

console.log('Generated', path.relative(root, output));
