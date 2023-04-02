import { FGStorage } from '@co2-storage/js-api'
import fs from 'fs'
import { program } from 'commander'
import * as dotenv from 'dotenv'

dotenv.config()

const authType = "pk"
const ipfsNodeType = "client"
const ipfsNodeAddr = "/dns4/web1.co2.storage/tcp/5002/https"
const fgApiUrl = "https://web1.co2.storage"

const fgStorage = new FGStorage({authType: authType, ipfsNodeType: ipfsNodeType, ipfsNodeAddr: ipfsNodeAddr, fgApiHost: fgApiUrl})

program
  .name('co2.storage')
  .description('Store power usage data on Protocol Labs CO2.Storage service.')
  .version('0.8.1');

program.command('upload')
  .description('Upload a data file to CO2.Storage')
  .argument('<string>', 'timestamp of reading')
  .argument('<string>', 'duration')
  .argument('<string>', 'power consumed in W')
  .argument('<string>', 'emissions')
  .option('-t, --template <string>', 'The CO2.Storage template to use to upload the data.');

program.parse()

const assetElements = [
  {  
    "name": "RunID",
    "value": process.env.RUN_ID
  },
  {
    "name": "TeamID",
    "value": process.env.TEAM_ID
  },
  {
    "name": "TeamName",
    "value": process.env.TEAM_NAME
  },
  {
    "name": "TeamDescription",
    "value": null
  },
  {
    "name": "ProjectID",
    "value": process.env.PROJECT_ID
  },
  {
    "name": "ProjectName",
    "value": process.env.PROJECT_NAME
  },
  {
    "name": "ProjectDescription",
    "value": null
  },
  {
    "name": "CloudProvider",
    "value": null
  },
  {
    "name": "ExperimentID",
    "value": process.env.EXPERIMENT_ID
  },
  {
    "name": "ExperimentName",
    "value": process.env.EXPERIMENT_NAME
  },
  {
    "name": "ExperimentDescription",
    "value": null
  },
  {
    "name": "CountryName",
    "value": process.env.COUNTRY_NAME
  },
  {
    "name": "CountryISOCode",
    "value": process.env.COUNTRY_ISO_CODE
  },
  {
    "name": "CountryRegion",
    "value": process.env.COUNTRY_REGION
  },
  {
    "name": "OnCloud",
    "value":false
  },
  {
    "name": "Timestamp",
    "value": Date(program.args[1])
  },
  {
    "name": "Description",
    "value": null
  },
  {
    "name": "Duration",
    "value": parseInt(program.args[2])
  },
  {
    "name": "Emissions",
    "value": parseFloat(program.args[3])
  },
  {
    "name": "EnergyConsumed",
    "value": parseFloat(program.args[4])
  },
];
console.log(assetElements);

/**
 * Add asset
 * parameters: { options } -> (assetElements:json, asset parent:string(CID), asset name:string, asset template:string(CID),
 *  upload start event callback, upload progress callback(bytes uploaded), upload end event callback,
 *  asset creation start event callback, asset creation end event callback)
 * 
 */

let addAssetResponse = await fgStorage.addAsset(
    assetElements,
    {
        parent: null,
        name: "libpwrm reading for " + program.args[1],
        description: "libpwrm asset",
        template: "bafyreiejmjeene2cbxp5rgxjflp4c5yh7a3lpg223g542srp3jgavxboui", 
        filesUploadStart: () => {
            console.log("Upload started")
        },
        filesUpload: async (bytes, path) => {
            console.log(`${bytes} uploaded`)
        },
        filesUploadEnd: () => {
            console.log("Upload finished")
        },
        createAssetStart: () => {
            console.log("Creating asset")
        },
        createAssetEnd: () => {
            console.log("Asset created")
        }
    },
    'sandbox'
)
if(addAssetResponse.error != null) {
  console.log("Add asset returned an error...");  
  console.log(addAssetResponse.error)
    await new Promise(reject => setTimeout(reject, 300));
    process.exit()
}

console.dir(addAssetResponse.result, {depth: null})

await new Promise(resolve => setTimeout(resolve, 1000));

// Exit program
process.exit()
