import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.StringTokenizer;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.mapreduce.JobContext;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.RecordReader;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.LineRecordReader;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;
import org.apache.hadoop.mapreduce.lib.input.NLineInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FileSystem;

public class sleepTime {

  public static class TokenizerMapper
       extends Mapper<Object, Text, Text, IntWritable>{

    private static final IntWritable one = new IntWritable(1);
    private String tweet = new String();
    private Text word = new Text();

    public void map(Object key, Text value, Context context) throws IOException, InterruptedException {
        //System.out.println(value.toString());
        //System.out.println("Howdy");
        String token = value.toString();
        //String [] parseLine = token.split("\n");
        if (!token.isEmpty()){
            
            tweet = tweet.concat(token + "\n");
        }
        else {
            Boolean foundKeyWord = false;
            //System.out.println(tweet);
            //System.out.println("Howdy here!");         
            String [] parseLine = tweet.split("\n");
            for (String str : parseLine){            
                if (str.length() > 0 && str.charAt(0) == 'W'){
                    String [] parseSpace = str.split(" ");
                    for (String s : parseSpace){
                        if (s.toLowerCase().equals("random")){  
                            System.out.println(s);
                            System.out.println("Howdy here!");
                            foundKeyWord = true;
                            break;
                        }
                    }
                } 
            }
            if (foundKeyWord){
                for (String str : parseLine){   
                    if (str.length() > 0 && str.charAt(0) == 'T'){ 
                        String [] parseSpace = tweet.split(" ");
                        String [] parseTime = parseSpace[1].split(":");
                        word.set(parseTime[0]);
                        context.write(word, one);
                    }
                }
            }
            tweet = "";
        }
    }
  }

  public static class IntSumReducer extends Reducer<Text,IntWritable,Text,IntWritable> {
    private IntWritable result = new IntWritable();

    public void reduce(Text key, Iterable<IntWritable> values, Context context) throws IOException, InterruptedException {
      int sum = 0;     
      System.out.println(key.toString());
      for (IntWritable val : values) {
        sum += val.get();
      }
      result.set(sum);
      context.write(key, result);
    }
  }

  public static void main(String[] args) throws Exception {
    Configuration conf = new Configuration();
    //conf.set("textinputformat.record.delimiter", "\n\n");
    Job job = Job.getInstance(conf, "sleep time count");
    job.setJarByClass(sleepTime.class);
    job.setMapperClass(TokenizerMapper.class);
    job.setCombinerClass(IntSumReducer.class);
    job.setReducerClass(IntSumReducer.class);
    job.setOutputKeyClass(Text.class);
    job.setOutputValueClass(IntWritable.class);
    FileInputFormat.addInputPath(job, new Path(args[0]));
    FileOutputFormat.setOutputPath(job, new Path(args[1]));
    System.exit(job.waitForCompletion(true) ? 0 : 1);
  }
}